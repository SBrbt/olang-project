/*
 * Semantic analysis: scopes, types, bindings, and layout metadata on the AST.
 * Consumes parser output; does not emit instructions. Code generation:
 * backend/codegen_x64.c. Boundaries: docs/internals/compiler-modules.md
 */
#include "sema.h"

#include "ast.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t hash_path_djb2(const char *s) {
  uint32_t h = 5381u;
  unsigned char c;
  if (!s) return 0u;
  while ((c = (unsigned char)*s++) != 0) h = ((h << 5u) + h) + (uint32_t)c;
  return h;
}

typedef struct SymSlot {
  char name[128];
  OlTypeRef ty;
  unsigned char is_file_global; /* 1: file-level let (addr uses global object, not stack) */
  unsigned char indirect;       /* 1: slot holds ptr; ty is element type (indirect binding) */
  uint32_t view_byte_off;      /* offset within shared stack/global blob (direct locals) */
} SymSlot;

#define MAX_SYM 512
#define MAX_SCOPE 128

typedef struct SemaCtx {
  OlProgram *prog;
  SymSlot syms[MAX_SYM];
  int sym_count;
  int scope_base[MAX_SCOPE];
  int scope_depth;
  int loop_depth;
  int ref_bind_depth; /* >0 while resolving inner of OL_EX_REF_BIND (bare find allowed) */
  char err[512];
} SemaCtx;

static void sema_err(SemaCtx *S, int line, const char *msg) { 
  if (line > 0) {
    snprintf(S->err, sizeof(S->err), "line %d: %s", line, msg); 
  } else {
    snprintf(S->err, sizeof(S->err), "%s", msg); 
  }
}

static int type_eq(const OlTypeRef *a, const OlTypeRef *b) {
  if (a->kind != b->kind) return 0;
  if (a->kind == OL_TY_ALIAS) return strcmp(a->alias_name, b->alias_name) == 0;
  if (a->kind == OL_TY_REF) {
    if (!a->ref_inner || !b->ref_inner) return 0;
    return type_eq(a->ref_inner, b->ref_inner);
  }
  return 1;
}

static void type_free_ref(OlTypeRef *t) {
  if (!t || t->kind != OL_TY_REF || !t->ref_inner) return;
  type_free_ref(t->ref_inner);
  free(t->ref_inner);
  t->ref_inner = NULL;
}

static void type_copy(OlTypeRef *dst, const OlTypeRef *src) {
  type_free_ref(dst);
  dst->kind = src->kind;
  snprintf(dst->alias_name, sizeof(dst->alias_name), "%s", src->alias_name);
  dst->ref_inner = NULL;
  if (src->kind == OL_TY_REF && src->ref_inner) {
    dst->ref_inner = (OlTypeRef *)malloc(sizeof(OlTypeRef));
    if (dst->ref_inner) {
      memset(dst->ref_inner, 0, sizeof(*dst->ref_inner));
      type_copy(dst->ref_inner, src->ref_inner);
    }
  }
}

static void type_make_ref(OlTypeRef *out, const OlTypeRef *elem) {
  type_free_ref(out);
  out->kind = OL_TY_REF;
  out->alias_name[0] = '\0';
  out->ref_inner = (OlTypeRef *)malloc(sizeof(OlTypeRef));
  if (!out->ref_inner) return;
  memset(out->ref_inner, 0, sizeof(*out->ref_inner));
  type_copy(out->ref_inner, elem);
}

static int type_is_ref(const OlTypeRef *t) { return t && t->kind == OL_TY_REF && t->ref_inner != NULL; }

/* R with element type void: location without static element type (find / @alloc); use <T> for a typed view. */
static int type_is_untyped_ref(const OlTypeRef *t) {
  return type_is_ref(t) && t->ref_inner && t->ref_inner->kind == OL_TY_VOID;
}

static void type_make_untyped_ref(OlTypeRef *out) {
  OlTypeRef ve;
  memset(&ve, 0, sizeof(ve));
  ve.kind = OL_TY_VOID;
  type_make_ref(out, &ve);
}

static const OlTypeRef *type_ref_elem(const OlTypeRef *t) {
  if (!type_is_ref(t)) return NULL;
  return t->ref_inner;
}

static int expr_is_value_ok(SemaCtx *S, OlExpr *e, const char *hint) {
  if (type_is_ref(&e->ty)) {
    sema_err(S, e->line, hint);
    return 0;
  }
  return 1;
}

static int type_is_int(const OlTypeRef *t) {
  return t->kind == OL_TY_I8 || t->kind == OL_TY_U8 ||
         t->kind == OL_TY_I16 || t->kind == OL_TY_U16 ||
         t->kind == OL_TY_I32 || t->kind == OL_TY_U32 ||
         t->kind == OL_TY_I64 || t->kind == OL_TY_U64;
}
static int type_is_signed_int(const OlTypeRef *t) {
  return t->kind == OL_TY_I8 || t->kind == OL_TY_I16 || t->kind == OL_TY_I32 || t->kind == OL_TY_I64;
}
static int type_is_unsigned_int(const OlTypeRef *t) {
  return t->kind == OL_TY_U8 || t->kind == OL_TY_U16 || t->kind == OL_TY_U32 || t->kind == OL_TY_U64;
}
static int type_is_float(const OlTypeRef *t) {
  return t->kind == OL_TY_F16 || t->kind == OL_TY_F32 || t->kind == OL_TY_F64;
}
static int type_is_binary(const OlTypeRef *t) {
  return t->kind == OL_TY_B8 || t->kind == OL_TY_B16 || t->kind == OL_TY_B32 || t->kind == OL_TY_B64;
}
static int type_is_scalar(const OlTypeRef *t) {
  return type_is_int(t) || type_is_float(t) || type_is_binary(t) || t->kind == OL_TY_PTR || t->kind == OL_TY_BOOL;
}
static int type_is_condition(const OlTypeRef *t) {
  return t->kind == OL_TY_BOOL;
}

static int type_is_valid(const OlProgram *p, const OlTypeRef *t) {
  if (t->kind == OL_TY_REF) return t->ref_inner != NULL && type_is_valid(p, t->ref_inner);
  if (t->kind == OL_TY_VOID || t->kind == OL_TY_BOOL ||
      t->kind == OL_TY_U8 || t->kind == OL_TY_I8 ||
      t->kind == OL_TY_U16 || t->kind == OL_TY_I16 ||
      t->kind == OL_TY_I32 || t->kind == OL_TY_U32 ||
      t->kind == OL_TY_I64 || t->kind == OL_TY_U64 ||
      t->kind == OL_TY_F16 || t->kind == OL_TY_F32 || t->kind == OL_TY_F64 ||
      t->kind == OL_TY_B8 || t->kind == OL_TY_B16 || t->kind == OL_TY_B32 || t->kind == OL_TY_B64 ||
      t->kind == OL_TY_PTR) return 1;
  if (t->kind != OL_TY_ALIAS) return 0;
  return ol_program_find_typedef(p, t->alias_name) >= 0;
}

static int type_from_name(const char *tn, OlTypeRef *out) {
  memset(out, 0, sizeof(*out));
  if (strcmp(tn, "bool") == 0) { out->kind = OL_TY_BOOL; return 1; }
  if (strcmp(tn, "u8") == 0) { out->kind = OL_TY_U8; return 1; }
  if (strcmp(tn, "i8") == 0) { out->kind = OL_TY_I8; return 1; }
  if (strcmp(tn, "u16") == 0) { out->kind = OL_TY_U16; return 1; }
  if (strcmp(tn, "i16") == 0) { out->kind = OL_TY_I16; return 1; }
  if (strcmp(tn, "i32") == 0) { out->kind = OL_TY_I32; return 1; }
  if (strcmp(tn, "u32") == 0) { out->kind = OL_TY_U32; return 1; }
  if (strcmp(tn, "i64") == 0) { out->kind = OL_TY_I64; return 1; }
  if (strcmp(tn, "u64") == 0) { out->kind = OL_TY_U64; return 1; }
  if (strcmp(tn, "f16") == 0) { out->kind = OL_TY_F16; return 1; }
  if (strcmp(tn, "f32") == 0) { out->kind = OL_TY_F32; return 1; }
  if (strcmp(tn, "f64") == 0) { out->kind = OL_TY_F64; return 1; }
  if (strcmp(tn, "b8") == 0) { out->kind = OL_TY_B8; return 1; }
  if (strcmp(tn, "b16") == 0) { out->kind = OL_TY_B16; return 1; }
  if (strcmp(tn, "b32") == 0) { out->kind = OL_TY_B32; return 1; }
  if (strcmp(tn, "b64") == 0) { out->kind = OL_TY_B64; return 1; }
  if (strcmp(tn, "ptr") == 0) { out->kind = OL_TY_PTR; return 1; }
  out->kind = OL_TY_ALIAS;
  snprintf(out->alias_name, sizeof(out->alias_name), "%s", tn);
  return 1;
}

static int resolve_field_type(const OlProgram *p, const char *struct_name, const char *field_name, OlTypeRef *out) {
  int ti = ol_program_find_typedef(p, struct_name);
  size_t i;
  if (ti < 0) return 0;
  if (p->typedefs[ti].kind != OL_TYPEDEF_STRUCT) return 0;
  for (i = 0; i < p->typedefs[ti].field_count; ++i) {
    if (strcmp(p->typedefs[ti].fields[i].name, field_name) == 0) {
      return type_from_name(p->typedefs[ti].fields[i].type_name, out);
    }
  }
  return 0;
}

static int resolve_array_elem_type(const OlProgram *p, const char *array_name, OlTypeRef *out) {
  int ti = ol_program_find_typedef(p, array_name);
  if (ti < 0 || p->typedefs[ti].kind != OL_TYPEDEF_ARRAY) return 0;
  return type_from_name(p->typedefs[ti].elem_type, out);
}

static int resolve_alias_kind(const OlProgram *p, const OlTypeRef *t, OlTypeDefKind *k) {
  int ti;
  if (t->kind != OL_TY_ALIAS) return 0;
  ti = ol_program_find_typedef(p, t->alias_name);
  if (ti < 0) return 0;
  *k = p->typedefs[ti].kind;
  return 1;
}

static uint32_t ty_size_bytes(const OlProgram *p, const OlTypeRef *t) {
  int k;
  switch (t->kind) {
    case OL_TY_VOID:
      return 0u;
    case OL_TY_BOOL:
    case OL_TY_U8:
    case OL_TY_I8:
    case OL_TY_B8:
      return 1u;
    case OL_TY_U16:
    case OL_TY_I16:
    case OL_TY_F16:
    case OL_TY_B16:
      return 2u;
    case OL_TY_I32:
    case OL_TY_U32:
    case OL_TY_F32:
    case OL_TY_B32:
      return 4u;
    case OL_TY_I64:
    case OL_TY_U64:
    case OL_TY_F64:
    case OL_TY_B64:
    case OL_TY_PTR:
      return 8u;
    case OL_TY_REF:
      return 8u;
    case OL_TY_ALIAS:
      k = ol_program_find_typedef(p, t->alias_name);
      if (k < 0) return 0u;
      return p->typedefs[k].size_bytes;
  }
  return 0u;
}

static int can_explicit_cast(OlProgram *p, const OlTypeRef *from, const OlTypeRef *to) {
  uint32_t sf, st;
  OlTypeRef fe;
  const OlTypeRef *fromv = from;
  if (from->kind == OL_TY_REF && from->ref_inner) {
    type_copy(&fe, from->ref_inner);
    fromv = &fe;
  }
  if (type_eq(fromv, to)) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 0; /* same type: no explicit cast */
  }
  sf = ty_size_bytes(p, fromv);
  st = ty_size_bytes(p, to);
  /* Same-width iN<->uN reinterpret (e.g. i32→u32): not implemented (runtime sign semantics undefined here). */
  if (sf > 0u && sf == st && type_is_int(fromv) && type_is_int(to) &&
      ((type_is_signed_int(fromv) && type_is_unsigned_int(to)) ||
       (type_is_unsigned_int(fromv) && type_is_signed_int(to)))) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 0;
  }
  /* Same-size non-float scalars (value cast). For storage views use multi-binding let with <T>addr / find. */
  if (sf > 0u && sf == st && fromv->kind != OL_TY_ALIAS && to->kind != OL_TY_ALIAS &&
      !type_is_float(fromv) && !type_is_float(to) && type_is_scalar(fromv) && type_is_scalar(to)) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 1;
  }
  /* uN -> uN */
  if (type_is_unsigned_int(fromv) && type_is_unsigned_int(to)) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 1;
  }
  /* iN -> iN */
  if (type_is_signed_int(fromv) && type_is_signed_int(to)) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 1;
  }
  /* bN -> bN */
  if (type_is_binary(fromv) && type_is_binary(to)) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 1;
  }
  /* fN -> fN */
  if (type_is_float(fromv) && type_is_float(to)) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 1;
  }
  /* fN <-> iN */
  if (type_is_float(fromv) && type_is_signed_int(to)) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 1;
  }
  if (type_is_signed_int(fromv) && type_is_float(to)) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 1;
  }
  /* fN <-> uN */
  if (type_is_float(fromv) && type_is_unsigned_int(to)) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 1;
  }
  if (type_is_unsigned_int(fromv) && type_is_float(to)) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 1;
  }
  /* bool -> uN/iN */
  if (fromv->kind == OL_TY_BOOL && (type_is_unsigned_int(to) || type_is_signed_int(to))) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 1;
  }
  /* uN/iN -> bool */
  if ((type_is_unsigned_int(fromv) || type_is_signed_int(fromv)) && to->kind == OL_TY_BOOL) {
    if (from->kind == OL_TY_REF) type_free_ref(&fe);
    return 1;
  }
  if (from->kind == OL_TY_REF) type_free_ref(&fe);
  return 0;
}

static int stmt_all_paths_return(OlStmt *s, int need_val);
static int stmt_list_all_paths_return(OlStmt *s, int need_val);

static int stmt_all_paths_return(OlStmt *s, int need_val) {
  if (!s) return 0;
  if (s->kind == OL_ST_BLOCK) return stmt_list_all_paths_return(s->u.block.first, need_val);
  if (s->kind == OL_ST_RETURN) return need_val ? (s->u.ret.val != NULL) : 1;
  if (s->kind == OL_ST_IF) {
    if (!s->u.if_.else_arm) return 0;
    return stmt_all_paths_return(s->u.if_.then_arm, need_val) && stmt_all_paths_return(s->u.if_.else_arm, need_val);
  }
  return 0;
}

static int stmt_list_all_paths_return(OlStmt *s, int need_val) {
  while (s) {
    if (s->kind == OL_ST_RETURN) return need_val ? (s->u.ret.val != NULL) : 1;
    if (s->kind == OL_ST_BLOCK) {
      if (stmt_list_all_paths_return(s->u.block.first, need_val)) return 1;
    } else if (s->kind == OL_ST_IF && s->u.if_.else_arm) {
      if (stmt_all_paths_return(s->u.if_.then_arm, need_val) && stmt_all_paths_return(s->u.if_.else_arm, need_val)) return 1;
    }
    s = s->next;
  }
  return 0;
}

static int func_body_returns(OlStmt *body) {
  if (!body || body->kind != OL_ST_BLOCK) return 0;
  /* need_val=0: valueless `ret` after e.g. tail call to platform stub. */
  return stmt_list_all_paths_return(body->u.block.first, 0);
}

static void enter_scope(SemaCtx *S) {
  if (S->scope_depth >= MAX_SCOPE) return;
  S->scope_base[S->scope_depth++] = S->sym_count;
}

static void exit_scope(SemaCtx *S) {
  if (S->scope_depth <= 0) return;
  S->sym_count = S->scope_base[--S->scope_depth];
}

static SymSlot *lookup_sym(SemaCtx *S, const char *name) {
  int i;
  for (i = S->sym_count - 1; i >= 0; --i) {
    if (strcmp(S->syms[i].name, name) == 0) return &S->syms[i];
  }
  return NULL;
}

static int exists_in_current_scope(SemaCtx *S, const char *name) {
  int base = S->scope_depth > 0 ? S->scope_base[S->scope_depth - 1] : 0;
  int i;
  for (i = base; i < S->sym_count; ++i) {
    if (strcmp(S->syms[i].name, name) == 0) return 1;
  }
  return 0;
}

static int bind_sym(SemaCtx *S, const char *name, const OlTypeRef *ty, int file_global, uint32_t view_byte_off) {
  if (S->sym_count >= MAX_SYM) return 0;
  if (exists_in_current_scope(S, name)) return 0;
  snprintf(S->syms[S->sym_count].name, sizeof(S->syms[S->sym_count].name), "%s", name);
  S->syms[S->sym_count].ty = *ty;
  S->syms[S->sym_count].is_file_global = file_global ? 1u : 0u;
  S->syms[S->sym_count].indirect = 0u;
  S->syms[S->sym_count].view_byte_off = view_byte_off;
  S->sym_count++;
  return 1;
}

/* Slot holds a pointer; ty is the logical (element) type for load/store (indirect binding). */
static int bind_sym_indirect(SemaCtx *S, const char *name, const OlTypeRef *ty, int file_global) {
  if (S->sym_count >= MAX_SYM) return 0;
  if (exists_in_current_scope(S, name)) return 0;
  snprintf(S->syms[S->sym_count].name, sizeof(S->syms[S->sym_count].name), "%s", name);
  S->syms[S->sym_count].ty = *ty;
  S->syms[S->sym_count].is_file_global = file_global ? 1u : 0u;
  S->syms[S->sym_count].indirect = 1u;
  S->syms[S->sym_count].view_byte_off = 0u;
  S->sym_count++;
  return 1;
}

static const OlFuncDef *find_func(const OlProgram *p, const char *name) {
  size_t i;
  for (i = 0; i < p->func_count; ++i) {
    if (strcmp(p->funcs[i].name, name) == 0) return &p->funcs[i];
  }
  return NULL;
}

static int signatures_match(const OlExternDecl *ex, const OlFuncDef *fn) {
  size_t k;
  if (!type_eq(&ex->ret, &fn->ret)) return 0;
  if (ex->param_count != fn->param_count) return 0;
  for (k = 0; k < ex->param_count; ++k) {
    if (!type_eq(&ex->params[k].ty, &fn->params[k].ty)) return 0;
  }
  return 1;
}

static int global_storage_is_rodata(const OlGlobalDef *g) {
  return g->section == OL_GSEC_RODATA;
}

/* True if e designates (a subobject of) a @rodata global. */
static int store_lvalue_is_rodata(SemaCtx *S, const OlExpr *e) {
  if (!e) return 0;
  switch (e->kind) {
    case OL_EX_VAR: {
      SymSlot *sl = lookup_sym(S, e->u.var_name);
      int gi = ol_program_find_global(S->prog, e->u.var_name);
      if (!sl || !sl->is_file_global || gi < 0) return 0;
      return global_storage_is_rodata(&S->prog->globals[(size_t)gi]);
    }
    case OL_EX_FIELD:
      return store_lvalue_is_rodata(S, e->u.field.obj);
    case OL_EX_INDEX:
      return store_lvalue_is_rodata(S, e->u.index_.arr);
    default:
      return 0;
  }
}

static int init_expr_is_const(const OlExpr *e) {
  if (!e) return 0;
  switch (e->kind) {
    case OL_EX_INT:
    case OL_EX_FLOAT:
    case OL_EX_BOOL:
    case OL_EX_CHAR:
    case OL_EX_STR:
      return 1;
    default:
      return 0;
  }
}

static const OlTypeRef *lookup_global_type(const OlProgram *p, const char *name) {
  int gi = ol_program_find_global(p, name);
  size_t bi;
  if (gi < 0) return NULL;
  for (bi = 0; bi < p->globals[(size_t)gi].binding_count; ++bi) {
    if (strcmp(p->globals[(size_t)gi].bindings[bi].name, name) == 0)
      return &p->globals[(size_t)gi].bindings[bi].ty;
  }
  return NULL;
}

/* Bytes at the pointee storage (element size); 0 if unknown (e.g. opaque ptr). */
static uint32_t sym_pointee_storage_bytes(SemaCtx *S, SymSlot *sl) {
  int tdi;
  if (!sl) return 0u;
  if (sl->indirect) return ty_size_bytes(S->prog, &sl->ty);
  if (sl->ty.kind == OL_TY_PTR) return 0u;
  if (sl->ty.kind == OL_TY_ALIAS) {
    tdi = ol_program_find_typedef(S->prog, sl->ty.alias_name);
    if (tdi >= 0) {
      if (S->prog->typedefs[(size_t)tdi].kind == OL_TYPEDEF_STRUCT ||
          S->prog->typedefs[(size_t)tdi].kind == OL_TYPEDEF_ARRAY)
        return S->prog->typedefs[(size_t)tdi].size_bytes;
    }
  }
  return ty_size_bytes(S->prog, &sl->ty);
}

static int typedef_is_aggregate(const OlProgram *p, const OlTypeRef *t, int *out_agg) {
  int tdi;
  *out_agg = 0;
  if (t->kind != OL_TY_ALIAS) return 1;
  tdi = ol_program_find_typedef(p, t->alias_name);
  if (tdi < 0) return 0;
  if (p->typedefs[(size_t)tdi].kind == OL_TYPEDEF_STRUCT || p->typedefs[(size_t)tdi].kind == OL_TYPEDEF_ARRAY)
    *out_agg = 1;
  return 1;
}

static int bind_global_def(SemaCtx *S, OlProgram *p, OlGlobalDef *gd) {
  size_t bi;
  uint32_t off = 0;
  for (bi = 0; bi < gd->binding_count; ++bi) {
    if (!bind_sym(S, gd->bindings[bi].name, &gd->bindings[bi].ty, 1, off)) return 0;
    off += ty_size_bytes(p, &gd->bindings[bi].ty);
  }
  return 1;
}

static int validate_global_def(OlProgram *p, OlGlobalDef *gd, char *err, size_t err_len) {
  size_t bi, bj;
  uint32_t sum_bits = 0;
  uint32_t bw;
  int is_aggregate = 0;
  OlExpr *init = NULL;
  if (!gd->ref_expr || gd->ref_expr->kind != OL_EX_ALLOC) {
    snprintf(err, err_len, "global let requires @data|@bss|@rodata|@section<bits>(...)");
    return 0;
  }
  if (gd->ref_expr->u.alloc_.alloc == OL_ALLOC_STACK) {
    snprintf(err, err_len, "@stack not allowed at file scope");
    return 0;
  }
  switch (gd->ref_expr->u.alloc_.alloc) {
    case OL_ALLOC_DATA:
      gd->section = OL_GSEC_DATA;
      break;
    case OL_ALLOC_BSS:
      gd->section = OL_GSEC_BSS;
      break;
    case OL_ALLOC_RODATA:
      gd->section = OL_GSEC_RODATA;
      break;
    case OL_ALLOC_CUSTOM:
      gd->section = OL_GSEC_CUSTOM;
      snprintf(gd->custom_section, sizeof(gd->custom_section), "%s", gd->ref_expr->u.alloc_.custom_section);
      break;
    default:
      snprintf(err, err_len, "invalid global allocator");
      return 0;
  }
  if (gd->ref_expr->u.alloc_.bitwidth_is_sizeof) {
    if (!type_is_valid(p, &gd->ref_expr->u.alloc_.sizeof_bw_ty) || gd->ref_expr->u.alloc_.sizeof_bw_ty.kind == OL_TY_VOID) {
      snprintf(err, err_len, "invalid sizeof type in global allocator width");
      return 0;
    }
    {
      uint32_t bb = ty_size_bytes(p, &gd->ref_expr->u.alloc_.sizeof_bw_ty) * 8u;
      if (bb == 0u) {
        snprintf(err, err_len, "global allocator sizeof width is zero");
        return 0;
      }
      gd->ref_expr->u.alloc_.bitwidth = bb;
      gd->ref_expr->u.alloc_.bitwidth_is_sizeof = 0;
    }
  }
  bw = gd->ref_expr->u.alloc_.bitwidth;
  init = gd->ref_expr->u.alloc_.init;
  if (gd->binding_count < 1 || gd->binding_count > OL_MAX_LET_BINDINGS) {
    snprintf(err, err_len, "invalid global binding count");
    return 0;
  }
  for (bi = 0; bi < gd->binding_count; ++bi) {
    for (bj = bi + 1; bj < gd->binding_count; ++bj) {
      if (strcmp(gd->bindings[bi].name, gd->bindings[bj].name) == 0) {
        snprintf(err, err_len, "duplicate name in global let");
        return 0;
      }
    }
  }
  for (bi = 0; bi < gd->binding_count; ++bi) {
    if (!gd->bindings[bi].has_ty) {
      snprintf(err, err_len, "global let requires a type in name<Type>");
      return 0;
    }
    if (!type_is_valid(p, &gd->bindings[bi].ty) || gd->bindings[bi].ty.kind == OL_TY_VOID) {
      snprintf(err, err_len, "invalid global type");
      return 0;
    }
  }
  if (gd->binding_count == 1) {
    if (!typedef_is_aggregate(p, &gd->bindings[0].ty, &is_aggregate)) {
      snprintf(err, err_len, "invalid global type");
      return 0;
    }
  } else {
    for (bi = 0; bi < gd->binding_count; ++bi) {
      int agg = 0;
      if (!typedef_is_aggregate(p, &gd->bindings[bi].ty, &agg)) {
        snprintf(err, err_len, "invalid global type");
        return 0;
      }
      if (agg) {
        snprintf(err, err_len, "multi-view global only supports scalar types");
        return 0;
      }
    }
  }
  for (bi = 0; bi < gd->binding_count; ++bi) {
    uint32_t b = ty_size_bytes(p, &gd->bindings[bi].ty) * 8u;
    if (b == 0) {
      snprintf(err, err_len, "global type has zero size");
      return 0;
    }
    sum_bits += b;
  }
  if (sum_bits != bw) {
    snprintf(err, err_len, "global view sizes must sum to bitwidth");
    return 0;
  }
  if (is_aggregate) {
    if (ty_size_bytes(p, &gd->bindings[0].ty) * 8u != bw) {
      snprintf(err, err_len, "global bitwidth does not match aggregate size");
      return 0;
    }
  }
  if (gd->section == OL_GSEC_BSS && init) {
    snprintf(err, err_len, "@bss global cannot have initializer");
    return 0;
  }
  if ((gd->section == OL_GSEC_DATA || gd->section == OL_GSEC_CUSTOM) && !init) {
    snprintf(err, err_len, "global in writable section needs initializer");
    return 0;
  }
  if (global_storage_is_rodata(gd)) {
    if (!init) {
      snprintf(err, err_len, "@rodata global requires initializer");
      return 0;
    }
    if (!init_expr_is_const(init)) {
      snprintf(err, err_len, "@rodata initializer must be constant");
      return 0;
    }
  }
  (void)init;
  return 1;
}

static int resolve_expr(SemaCtx *S, OlExpr *e);

static int resolve_expr(SemaCtx *S, OlExpr *e) {
  switch (e->kind) {
    case OL_EX_INT:
      switch (e->u.int_.int_suffix) {
        case 1: e->ty.kind = OL_TY_I32; break;
        case 2: e->ty.kind = OL_TY_I64; break;
        case 3: e->ty.kind = OL_TY_U8; break;
        case 4: e->ty.kind = OL_TY_U32; break;
        case 5: e->ty.kind = OL_TY_U64; break;
        case 6: e->ty.kind = OL_TY_I8; break;
        case 7: e->ty.kind = OL_TY_I16; break;
        case 8: e->ty.kind = OL_TY_U16; break;
        case 9: e->ty.kind = OL_TY_B8; break;
        case 10: e->ty.kind = OL_TY_B16; break;
        case 11: e->ty.kind = OL_TY_B32; break;
        case 12: e->ty.kind = OL_TY_B64; break;
        default: e->ty.kind = OL_TY_I32; break;
      }
      return 1;
    case OL_EX_FLOAT:
      switch (e->u.float_.float_suffix) {
        case 1: e->ty.kind = OL_TY_F32; break;
        case 2: e->ty.kind = OL_TY_F16; break;
        default: e->ty.kind = OL_TY_F64; break;
      }
      return 1;
    case OL_EX_BOOL:
      e->ty.kind = OL_TY_BOOL;
      return 1;
    case OL_EX_CHAR:
      e->ty.kind = OL_TY_U8;
      return 1;
    case OL_EX_STR:
      e->ty.kind = OL_TY_PTR;
      return 1;
    case OL_EX_VAR: {
      SymSlot *sl = lookup_sym(S, e->u.var_name);
      if (!sl) {
        sema_err(S, e->line, "unknown variable");
        return 0;
      }
      type_make_ref(&e->ty, &sl->ty);
      return 1;
    }
    case OL_EX_NEG:
      if (!resolve_expr(S, e->u.neg.inner)) return 0;
      if (!expr_is_value_ok(S, e->u.neg.inner, "negation operand must be a value (use load<...> for storage)")) return 0;
      if (!type_is_int(&e->u.neg.inner->ty) && !type_is_float(&e->u.neg.inner->ty) && !type_is_binary(&e->u.neg.inner->ty)) {
        sema_err(S, e->line, "negation requires int, float, or binary operand");
        return 0;
      }
      type_copy(&e->ty, &e->u.neg.inner->ty);
      return 1;
    case OL_EX_NOT:
      if (!resolve_expr(S, e->u.not_.inner)) return 0;
      if (!expr_is_value_ok(S, e->u.not_.inner, "! operand must be a value")) return 0;
      if (e->u.not_.inner->ty.kind != OL_TY_BOOL) {
        sema_err(S, e->line, "! requires bool operand");
        return 0;
      }
      e->ty.kind = OL_TY_BOOL;
      return 1;
    case OL_EX_BNOT:
      if (!resolve_expr(S, e->u.bnot.inner)) return 0;
      if (!expr_is_value_ok(S, e->u.bnot.inner, "~ operand must be a value")) return 0;
      if (!type_is_binary(&e->u.bnot.inner->ty)) {
        sema_err(S, e->line, "~ requires binary type operand");
        return 0;
      }
      type_copy(&e->ty, &e->u.bnot.inner->ty);
      return 1;
    case OL_EX_BINARY: {
      OlBinOp op = e->u.binary.op;
      if (!resolve_expr(S, e->u.binary.left) || !resolve_expr(S, e->u.binary.right)) return 0;
      if (!expr_is_value_ok(S, e->u.binary.left, "binary operand must be a value (use load<...> for storage)")) return 0;
      if (!expr_is_value_ok(S, e->u.binary.right, "binary operand must be a value (use load<...> for storage)")) return 0;
      /* logical && || : strict bool */
      if (op == OL_BIN_AND || op == OL_BIN_OR) {
        if (e->u.binary.left->ty.kind != OL_TY_BOOL || e->u.binary.right->ty.kind != OL_TY_BOOL) {
          sema_err(S, e->line, "&& / || require bool operands");
          return 0;
        }
        e->ty.kind = OL_TY_BOOL;
        return 1;
      }
      /* bitwise ops: strict binary, same type */
      if (op == OL_BIN_BAND || op == OL_BIN_BOR || op == OL_BIN_BXOR || op == OL_BIN_SHL || op == OL_BIN_SHR) {
        if (!type_is_binary(&e->u.binary.left->ty) || !type_is_binary(&e->u.binary.right->ty)) {
          sema_err(S, e->line, "bitwise ops require binary type operands");
          return 0;
        }
        if (!type_eq(&e->u.binary.left->ty, &e->u.binary.right->ty)) {
          sema_err(S, e->line, "bitwise operand type mismatch");
          return 0;
        }
        type_copy(&e->ty, &e->u.binary.left->ty);
        return 1;
      }
      /* all other ops require same type */
      if (!type_eq(&e->u.binary.left->ty, &e->u.binary.right->ty)) {
        sema_err(S, e->line, "binary operand type mismatch");
        return 0;
      }
      /* == != : all scalar types */
      if (op == OL_BIN_EQ || op == OL_BIN_NE) {
        if (!type_is_scalar(&e->u.binary.left->ty)) {
          sema_err(S, e->line, "== / != require scalar operands");
          return 0;
        }
        e->ty.kind = OL_TY_BOOL;
        return 1;
      }
      /* < > <= >= : int and float only (not binary) */
      if (op == OL_BIN_LT || op == OL_BIN_GT || op == OL_BIN_LE || op == OL_BIN_GE) {
        if (!type_is_int(&e->u.binary.left->ty) && !type_is_float(&e->u.binary.left->ty)) {
          sema_err(S, e->line, "ordered compare requires int or float operands");
          return 0;
        }
        e->ty.kind = OL_TY_BOOL;
        return 1;
      }
      /* + - * : int, float, binary */
      if (op == OL_BIN_ADD || op == OL_BIN_SUB || op == OL_BIN_MUL) {
        if (!type_is_int(&e->u.binary.left->ty) && !type_is_float(&e->u.binary.left->ty) && !type_is_binary(&e->u.binary.left->ty)) {
          sema_err(S, e->line, "arithmetic requires int, float, or binary operands");
          return 0;
        }
        type_copy(&e->ty, &e->u.binary.left->ty);
        return 1;
      }
      /* / : int, float (no %), binary */
      if (op == OL_BIN_DIV) {
        if (!type_is_int(&e->u.binary.left->ty) && !type_is_float(&e->u.binary.left->ty) && !type_is_binary(&e->u.binary.left->ty)) {
          sema_err(S, e->line, "division requires int, float, or binary operands");
          return 0;
        }
        type_copy(&e->ty, &e->u.binary.left->ty);
        return 1;
      }
      /* % : int and binary only (not float) */
      if (op == OL_BIN_MOD) {
        if (!type_is_int(&e->u.binary.left->ty) && !type_is_binary(&e->u.binary.left->ty)) {
          sema_err(S, e->line, "modulo requires int or binary operands");
          return 0;
        }
        type_copy(&e->ty, &e->u.binary.left->ty);
        return 1;
      }
      sema_err(S, e->line, "unknown binary op");
      return 0;
    }
    case OL_EX_CALL: {
      const OlExternDecl *ex = ol_program_get_extern(S->prog, e->u.call.callee);
      const OlFuncDef *fd = find_func(S->prog, e->u.call.callee);
      size_t i;
      if (ex) {
        if (e->u.call.arg_count != ex->param_count) {
          sema_err(S, e->line, "wrong arg count");
          return 0;
        }
        for (i = 0; i < ex->param_count; ++i) {
          if (!resolve_expr(S, e->u.call.args[i])) return 0;
          if (!expr_is_value_ok(S, e->u.call.args[i], "call argument must be a value")) return 0;
          if (!type_eq(&e->u.call.args[i]->ty, &ex->params[i].ty)) {
            sema_err(S, e->u.call.args[i]->line, "extern arg type mismatch");
            return 0;
          }
        }
        type_copy(&e->ty, &ex->ret);
        return 1;
      }
      if (fd) {
        if (e->u.call.arg_count != fd->param_count) {
          sema_err(S, e->line, "wrong arg count (fn)");
          return 0;
        }
        for (i = 0; i < fd->param_count; ++i) {
          if (!resolve_expr(S, e->u.call.args[i])) return 0;
          if (!expr_is_value_ok(S, e->u.call.args[i], "call argument must be a value")) return 0;
          if (!type_eq(&e->u.call.args[i]->ty, &fd->params[i].ty)) {
            sema_err(S, e->u.call.args[i]->line, "fn arg type mismatch");
            return 0;
          }
        }
        type_copy(&e->ty, &fd->ret);
        return 1;
      }
      sema_err(S, e->line, "unknown callee");
      return 0;
    }
    case OL_EX_CAST: {
      uint32_t csz, tsz;
      const OlTypeRef *src_sz;
      if (!resolve_expr(S, e->u.cast_.inner)) return 0;
      if (type_is_untyped_ref(&e->u.cast_.inner->ty)) {
        sema_err(S, e->line, "cannot cast untyped reference; use <T>... to attach a type first");
        return 0;
      }
      if (!type_is_valid(S->prog, &e->u.cast_.to) || e->u.cast_.to.kind == OL_TY_VOID) {
        sema_err(S, e->line, "invalid cast target type");
        return 0;
      }
      src_sz = type_is_ref(&e->u.cast_.inner->ty) ? type_ref_elem(&e->u.cast_.inner->ty) : &e->u.cast_.inner->ty;
      csz = ty_size_bytes(S->prog, src_sz);
      tsz = ty_size_bytes(S->prog, &e->u.cast_.to);
      if (csz > 0u && csz == tsz && e->u.cast_.inner->kind == OL_EX_VAR &&
          e->u.cast_.to.kind != OL_TY_ALIAS && src_sz->kind != OL_TY_ALIAS &&
          !type_is_float(src_sz) && !type_is_float(&e->u.cast_.to) &&
          type_is_scalar(src_sz) && type_is_scalar(&e->u.cast_.to)) {
        sema_err(S, e->line,
                  "same-width rebind of a variable: use `let n<T> <T>addr name`");
        return 0;
      }
      if (!can_explicit_cast(S->prog, &e->u.cast_.inner->ty, &e->u.cast_.to)) {
        sema_err(S, e->line, "invalid cast");
        return 0;
      }
      type_copy(&e->ty, &e->u.cast_.to);
      return 1;
    }
    case OL_EX_REF_BIND: {
      S->ref_bind_depth++;
      if (!resolve_expr(S, e->u.ref_bind.inner)) {
        S->ref_bind_depth--;
        return 0;
      }
      S->ref_bind_depth--;
      if (e->u.ref_bind.to.kind == OL_TY_VOID) {
        if (e->u.ref_bind.inner->ty.kind == OL_TY_PTR) {
          type_copy(&e->ty, &e->u.ref_bind.inner->ty);
          return 1;
        }
        if (type_is_untyped_ref(&e->u.ref_bind.inner->ty)) {
          e->ty.kind = OL_TY_PTR;
          return 1;
        }
        sema_err(S, e->line, "<void> ref expects ptr or untyped reference (find / @alloc)");
        return 0;
      }
      if (!type_is_valid(S->prog, &e->u.ref_bind.to)) {
        sema_err(S, e->line, "invalid ref bind type");
        return 0;
      }
      if (e->u.ref_bind.inner->ty.kind != OL_TY_PTR && !type_is_untyped_ref(&e->u.ref_bind.inner->ty) &&
          !type_is_ref(&e->u.ref_bind.inner->ty)) {
        sema_err(S, e->line, "typed ref bind expects ptr, untyped reference, or typed reference");
        return 0;
      }
      if (type_is_ref(&e->u.ref_bind.inner->ty) && !type_is_untyped_ref(&e->u.ref_bind.inner->ty)) {
        const OlTypeRef *ein = type_ref_elem(&e->u.ref_bind.inner->ty);
        uint32_t si = ty_size_bytes(S->prog, ein);
        uint32_t st = ty_size_bytes(S->prog, &e->u.ref_bind.to);
        if (si == 0u || st == 0u || si != st) {
          sema_err(S, e->line, "ref view requires same element size as inner reference");
          return 0;
        }
      }
      type_make_ref(&e->ty, &e->u.ref_bind.to);
      return 1;
    }
    case OL_EX_FIND: {
      if (e->u.find_.inner && e->u.find_.inner->kind == OL_EX_FIND) {
        sema_err(S, e->line, "cannot nest find");
        return 0;
      }
      if (!resolve_expr(S, e->u.find_.inner)) return 0;
      if (e->u.find_.inner->ty.kind != OL_TY_PTR) {
        sema_err(S, e->line,
                  "find expects a ptr value (e.g. string literal, or load<x> where x holds ptr)");
        return 0;
      }
      if (S->ref_bind_depth == 0) {
        sema_err(S, e->line, "bare find<...> not allowed; use <T>find<...> in let/ref");
        return 0;
      }
      type_make_untyped_ref(&e->ty);
      return 1;
    }
    case OL_EX_LOAD: {
      if (!resolve_expr(S, e->u.load_.inner)) return 0;
      if (!type_is_ref(&e->u.load_.inner->ty)) {
        sema_err(S, e->line, "load expects a reference (name, field, index, or <T>find<...>)");
        return 0;
      }
      if (type_ref_elem(&e->u.load_.inner->ty)->kind == OL_TY_VOID) {
        sema_err(S, e->line, "cannot load untyped reference; use <T>... for a typed view first");
        return 0;
      }
      type_copy(&e->ty, type_ref_elem(&e->u.load_.inner->ty));
      return 1;
    }
    case OL_EX_SIZEOF_TY: {
      if (!type_is_valid(S->prog, &e->u.sizeof_ty.ty) || e->u.sizeof_ty.ty.kind == OL_TY_VOID) {
        sema_err(S, e->line, "invalid sizeof type");
        return 0;
      }
      e->ty.kind = OL_TY_U64;
      return 1;
    }
    case OL_EX_ALLOC: {
      if (e->u.alloc_.bitwidth_is_sizeof) {
        if (!type_is_valid(S->prog, &e->u.alloc_.sizeof_bw_ty) || e->u.alloc_.sizeof_bw_ty.kind == OL_TY_VOID) {
          sema_err(S, e->line, "invalid sizeof type in allocator width");
          return 0;
        }
        {
          uint32_t b = ty_size_bytes(S->prog, &e->u.alloc_.sizeof_bw_ty) * 8u;
          if (b == 0u) {
            sema_err(S, e->line, "allocator sizeof width is zero");
            return 0;
          }
          e->u.alloc_.bitwidth = b;
          e->u.alloc_.bitwidth_is_sizeof = 0;
        }
      }
      if (e->u.alloc_.init && !resolve_expr(S, e->u.alloc_.init)) return 0;
      type_make_untyped_ref(&e->ty);
      return 1;
    }
    case OL_EX_ADDR: {
      SymSlot *sl = lookup_sym(S, e->u.addr.name);
      if (sl) {
        e->u.addr.addr_kind = sl->is_file_global ? OL_ADDR_GLOBAL : OL_ADDR_LOCAL;
        e->ty.kind = OL_TY_PTR;
        return 1;
      }
      if (ol_program_find_extern(S->prog, e->u.addr.name) >= 0 || find_func(S->prog, e->u.addr.name)) {
        e->u.addr.addr_kind = OL_ADDR_SYMBOL;
        e->ty.kind = OL_TY_PTR;
        return 1;
      }
      sema_err(S, e->line, "addr of unknown (need local, global, extern, or fn)");
      return 0;
    }
    case OL_EX_FIELD: {
      OlTypeRef st;
      OlTypeRef fty;
      OlTypeDefKind kind;
      if (!resolve_expr(S, e->u.field.obj)) return 0;
      if (!type_is_ref(&e->u.field.obj->ty)) {
        sema_err(S, e->line, "field access requires struct reference");
        return 0;
      }
      memset(&st, 0, sizeof(st));
      type_copy(&st, type_ref_elem(&e->u.field.obj->ty));
      if (!resolve_alias_kind(S->prog, &st, &kind) || kind != OL_TYPEDEF_STRUCT) {
        sema_err(S, e->line, "field on non-struct");
        return 0;
      }
      if (!resolve_field_type(S->prog, st.alias_name, e->u.field.field, &fty)) {
        sema_err(S, e->line, "unknown field");
        return 0;
      }
      type_free_ref(&st);
      type_make_ref(&e->ty, &fty);
      return 1;
    }
    case OL_EX_INDEX: {
      OlTypeRef at;
      OlTypeRef elty;
      OlTypeDefKind kind;
      if (!resolve_expr(S, e->u.index_.arr)) return 0;
      if (!resolve_expr(S, e->u.index_.index_expr)) return 0;
      if (!type_is_int(&e->u.index_.index_expr->ty)) {
        sema_err(S, e->line, "array index must be integer");
        return 0;
      }
      if (!expr_is_value_ok(S, e->u.index_.index_expr, "index must be a value")) return 0;
      if (!type_is_ref(&e->u.index_.arr->ty)) {
        sema_err(S, e->line, "subscript requires array reference");
        return 0;
      }
      memset(&at, 0, sizeof(at));
      type_copy(&at, type_ref_elem(&e->u.index_.arr->ty));
      if (!resolve_alias_kind(S->prog, &at, &kind) || kind != OL_TYPEDEF_ARRAY) {
        sema_err(S, e->line, "index on non-array");
        return 0;
      }
      if (ol_program_find_typedef(S->prog, at.alias_name) < 0) {
        sema_err(S, e->line, "unknown array type");
        return 0;
      }
      if (!resolve_array_elem_type(S->prog, at.alias_name, &elty)) {
        sema_err(S, e->line, "bad array elem type");
        return 0;
      }
      type_free_ref(&at);
      type_make_ref(&e->ty, &elty);
      return 1;
    }
    default:
      sema_err(S, e->line, "bad expr");
      return 0;
  }
}

static int check_stmt(SemaCtx *S, const OlFuncDef *fn, OlStmt *s);

/* let name1<T1> ... let nameN<TN> rhs_ref: rhs must be a reference; innermost name is next to rhs. */
static int check_let_ref_chain_views(SemaCtx *S, OlStmt *s) {
  OlExpr *cur;
  OlExpr *layers;
  OlExpr rb;
  size_t n = s->u.let_.binding_count;
  size_t i;
  size_t idx;
  OlLetBinding *bd = s->u.let_.bindings;
  if (n < 1u || n > OL_MAX_LET_BINDINGS) return 0;
  cur = s->u.let_.ref_expr;
  layers = (OlExpr *)calloc(n, sizeof(OlExpr));
  if (!layers) return 0;
  if (!resolve_expr(S, cur)) {
    free(layers);
    return 0;
  }
  if (!type_is_ref(&cur->ty)) {
    sema_err(S, s->line, "let ref-chain needs a reference on the right");
    free(layers);
    return 0;
  }
  for (i = n; i > 0u; --i) {
    idx = i - 1u;
    if (!bd[idx].has_ty) {
      sema_err(S, s->line, "let requires name<Type> with a type");
      free(layers);
      return 0;
    }
    if (lookup_global_type(S->prog, bd[idx].name)) {
      sema_err(S, s->line, "let name shadows global");
      free(layers);
      return 0;
    }
    if (!type_is_valid(S->prog, &bd[idx].ty) || bd[idx].ty.kind == OL_TY_VOID) {
      sema_err(S, s->line, "invalid let type");
      free(layers);
      return 0;
    }
    memset(&rb, 0, sizeof(rb));
    rb.kind = OL_EX_REF_BIND;
    rb.line = s->line;
    type_copy(&rb.u.ref_bind.to, &bd[idx].ty);
    rb.u.ref_bind.inner = cur;
    if (!resolve_expr(S, &rb)) {
      free(layers);
      return 0;
    }
    if (!bind_sym_indirect(S, bd[idx].name, &bd[idx].ty, 0)) {
      sema_err(S, s->line, "duplicate let or oom");
      free(layers);
      return 0;
    }
    memset(&layers[idx], 0, sizeof(OlExpr));
    layers[idx].kind = OL_EX_VAR;
    layers[idx].line = s->line;
    snprintf(layers[idx].u.var_name, sizeof(layers[idx].u.var_name), "%s", bd[idx].name);
    if (!resolve_expr(S, &layers[idx])) {
      free(layers);
      return 0;
    }
    cur = &layers[idx];
  }
  free(layers);
  return 1;
}

static int check_stmt(SemaCtx *S, const OlFuncDef *fn, OlStmt *s) {
  while (s) {
    switch (s->kind) {
      case OL_ST_BLOCK:
        enter_scope(S);
        if (!check_stmt(S, fn, s->u.block.first)) {
          exit_scope(S);
          return 0;
        }
        exit_scope(S);
        break;
      case OL_ST_LET: {
        OlExpr *re = s->u.let_.ref_expr;
        size_t bi;
        uint32_t sum_bits = 0;
        uint32_t bw;
        OlExpr *init_e;
        int is_aggregate = 0;
        if (!re) {
          sema_err(S, s->line, "let requires @stack<...>(...) or a ref expression");
          return 0;
        }
        if (re->kind == OL_EX_REF_BIND) {
          if (s->u.let_.binding_count != 1) {
            sema_err(S, s->line, "let with <...>ref allows only one binding");
            return 0;
          }
          if (!s->u.let_.bindings[0].has_ty) {
            sema_err(S, s->line, "let requires name<Type> with a type");
            return 0;
          }
          if (re->u.ref_bind.to.kind != OL_TY_VOID &&
              !type_eq(&re->u.ref_bind.to, &s->u.let_.bindings[0].ty)) {
            sema_err(S, s->line, "let binding type must match inner <Type> ref");
            return 0;
          }
          for (bi = 0; bi < s->u.let_.binding_count; ++bi) {
            if (lookup_global_type(S->prog, s->u.let_.bindings[bi].name)) {
              sema_err(S, s->line, "let name shadows global");
              return 0;
            }
            if (!type_is_valid(S->prog, &s->u.let_.bindings[bi].ty) || s->u.let_.bindings[bi].ty.kind == OL_TY_VOID) {
              sema_err(S, s->line, "invalid let type");
              return 0;
            }
          }
          if (!resolve_expr(S, re)) return 0;
          if (re->u.ref_bind.to.kind != OL_TY_VOID && re->u.ref_bind.inner->kind == OL_EX_ADDR) {
            SymSlot *csl = lookup_sym(S, re->u.ref_bind.inner->u.addr.name);
            uint32_t psz, tsz;
            if (csl) {
              psz = sym_pointee_storage_bytes(S, csl);
              tsz = ty_size_bytes(S->prog, &re->u.ref_bind.to);
              if (psz != 0u && psz != tsz) {
                sema_err(S, s->line, "indirect let: binding type size must match addressed storage");
                return 0;
              }
            }
          }
          if (!bind_sym_indirect(S, s->u.let_.bindings[0].name, &s->u.let_.bindings[0].ty, 0)) {
            sema_err(S, s->line, "duplicate let or oom");
            return 0;
          }
          break;
        }
        if (re->kind == OL_EX_VAR) {
          if (!check_let_ref_chain_views(S, s)) return 0;
          break;
        }
        if (re->kind != OL_EX_ALLOC) {
          sema_err(S, s->line, "local let requires @stack<...>(...) or <[Type]>ref (e.g. <i32>find<...>)");
          return 0;
        }
        if (re->u.alloc_.alloc != OL_ALLOC_STACK) {
          sema_err(S, s->line, "local let must be @stack<bits>(...)");
          return 0;
        }
        if (!resolve_expr(S, re)) return 0;
        bw = re->u.alloc_.bitwidth;
        init_e = re->u.alloc_.init;
        if (s->u.let_.binding_count == 0 || s->u.let_.binding_count > OL_MAX_LET_BINDINGS) {
          sema_err(S, s->line, "invalid let binding count");
          return 0;
        }
        for (bi = 0; bi < s->u.let_.binding_count; ++bi) {
          if (lookup_global_type(S->prog, s->u.let_.bindings[bi].name)) {
            sema_err(S, s->line, "let name shadows global");
            return 0;
          }
          if (!s->u.let_.bindings[bi].has_ty) {
            sema_err(S, s->line, "let requires a type in name<Type>");
            return 0;
          }
          if (!type_is_valid(S->prog, &s->u.let_.bindings[bi].ty) || s->u.let_.bindings[bi].ty.kind == OL_TY_VOID) {
            sema_err(S, s->line, "invalid let type");
            return 0;
          }
        }
        if (s->u.let_.binding_count == 1) {
          OlTypeRef *t0 = &s->u.let_.bindings[0].ty;
          if (t0->kind == OL_TY_ALIAS) {
            int tdi = ol_program_find_typedef(S->prog, t0->alias_name);
            if (tdi >= 0) {
              OlTypeDefKind tdk = S->prog->typedefs[(size_t)tdi].kind;
              if (tdk == OL_TYPEDEF_STRUCT || tdk == OL_TYPEDEF_ARRAY) {
                is_aggregate = 1;
              }
            }
          }
        } else {
          for (bi = 0; bi < s->u.let_.binding_count; ++bi) {
            OlTypeRef *t = &s->u.let_.bindings[bi].ty;
            if (t->kind == OL_TY_ALIAS) {
              int tdi = ol_program_find_typedef(S->prog, t->alias_name);
              if (tdi >= 0) {
                OlTypeDefKind tdk = S->prog->typedefs[(size_t)tdi].kind;
                if (tdk == OL_TYPEDEF_STRUCT || tdk == OL_TYPEDEF_ARRAY) {
                  sema_err(S, s->line, "multi-view let only supports scalar types");
                  return 0;
                }
              }
            }
          }
        }
        for (bi = 0; bi < s->u.let_.binding_count; ++bi) {
          uint32_t b = ty_size_bytes(S->prog, &s->u.let_.bindings[bi].ty) * 8u;
          if (b == 0) {
            sema_err(S, s->line, "let type has zero size");
            return 0;
          }
          sum_bits += b;
        }
        if (sum_bits != bw) {
          sema_err(S, s->line, "let view sizes must sum to bitwidth");
          return 0;
        }
        if (is_aggregate) {
          OlTypeRef *t0 = &s->u.let_.bindings[0].ty;
          uint32_t expect_bits = ty_size_bytes(S->prog, t0) * 8u;
          if (expect_bits != bw) {
            sema_err(S, s->line, "let bitwidth does not match aggregate size");
            return 0;
          }
          if (init_e) {
            if (!resolve_expr(S, init_e)) return 0;
            if (!type_eq(&init_e->ty, t0)) {
              sema_err(S, s->line, "let type mismatch");
              return 0;
            }
          }
          if (!bind_sym(S, s->u.let_.bindings[0].name, t0, 0, 0)) {
            sema_err(S, s->line, "duplicate let or oom");
            return 0;
          }
        } else {
          if (!init_e) {
            sema_err(S, s->line, "scalar let requires initializer");
            return 0;
          }
          if (!resolve_expr(S, init_e)) return 0;
          if (ty_size_bytes(S->prog, &init_e->ty) * 8u != bw) {
            sema_err(S, s->line, "initializer bit width must match let bitwidth");
            return 0;
          }
          if (s->u.let_.binding_count == 1 &&
              !type_eq(&init_e->ty, &s->u.let_.bindings[0].ty)) {
            sema_err(S, s->line, "let type mismatch");
            return 0;
          }
          {
            uint32_t off = 0;
            for (bi = 0; bi < s->u.let_.binding_count; ++bi) {
              uint32_t nb = ty_size_bytes(S->prog, &s->u.let_.bindings[bi].ty);
              if (!bind_sym(S, s->u.let_.bindings[bi].name, &s->u.let_.bindings[bi].ty, 0, off)) {
                sema_err(S, s->line, "duplicate let or oom");
                return 0;
              }
              off += nb;
            }
          }
        }
        break;
      }
      case OL_ST_STORE: {
        OlExpr *t = s->u.store_.target;
        if (!t) {
          sema_err(S, s->line, "store: missing target");
          return 0;
        }
        if (!resolve_expr(S, t)) return 0;
        if (!type_is_ref(&t->ty)) {
          sema_err(S, s->line, "store target must be a reference");
          return 0;
        }
        if (type_ref_elem(&t->ty)->kind == OL_TY_VOID) {
          sema_err(S, s->line, "cannot store to untyped reference; use <T>... for a typed view first");
          return 0;
        }
        if (store_lvalue_is_rodata(S, t)) {
          sema_err(S, s->line, "cannot store to @rodata global");
          return 0;
        }
        if (!resolve_expr(S, s->u.store_.val)) return 0;
        {
          const OlTypeRef *tel = type_ref_elem(&t->ty);
          int aggcpy = 0;
          if (type_is_ref(&s->u.store_.val->ty) && type_eq(tel, type_ref_elem(&s->u.store_.val->ty)) &&
              typedef_is_aggregate(S->prog, tel, &aggcpy) && aggcpy) {
            /* aggregate ref-to-ref copy: store<dst, src> */
          } else if (!expr_is_value_ok(S, s->u.store_.val, "store value must be a value (or matching aggregate ref)")) {
            return 0;
          }
        }
        if (!type_eq(type_ref_elem(&t->ty), type_is_ref(&s->u.store_.val->ty) ? type_ref_elem(&s->u.store_.val->ty) : &s->u.store_.val->ty)) {
          sema_err(S, s->line, "store type mismatch");
          return 0;
        }
        break;
      }
      case OL_ST_IF:
        if (!resolve_expr(S, s->u.if_.cond)) return 0;
        if (!expr_is_value_ok(S, s->u.if_.cond, "if condition must be a value")) return 0;
        if (!type_is_condition(&s->u.if_.cond->ty)) {
          sema_err(S, s->line, "if condition must be bool");
          return 0;
        }
        enter_scope(S);
        if (!check_stmt(S, fn, s->u.if_.then_arm)) {
          exit_scope(S);
          return 0;
        }
        exit_scope(S);
        if (s->u.if_.else_arm) {
          enter_scope(S);
          if (!check_stmt(S, fn, s->u.if_.else_arm)) {
            exit_scope(S);
            return 0;
          }
          exit_scope(S);
        }
        break;
      case OL_ST_WHILE:
        if (!resolve_expr(S, s->u.while_.cond)) return 0;
        if (!expr_is_value_ok(S, s->u.while_.cond, "while condition must be a value")) return 0;
        if (!type_is_condition(&s->u.while_.cond->ty)) {
          sema_err(S, s->line, "while condition must be bool");
          return 0;
        }
        enter_scope(S);
        S->loop_depth++;
        if (!check_stmt(S, fn, s->u.while_.body)) {
          S->loop_depth--;
          exit_scope(S);
          return 0;
        }
        S->loop_depth--;
        exit_scope(S);
        break;
      case OL_ST_BREAK:
        if (S->loop_depth <= 0) { sema_err(S, s->line, "break outside loop"); return 0; }
        break;
      case OL_ST_CONTINUE:
        if (S->loop_depth <= 0) { sema_err(S, s->line, "continue outside loop"); return 0; }
        break;
      case OL_ST_RETURN:
        if (fn->ret.kind == OL_TY_VOID) {
          if (s->u.ret.val) {
            sema_err(S, s->line, "void return with value");
            return 0;
          }
        } else {
          /* Valueless return is allowed (e.g. tail call to asm stub; rax unchanged). */
          if (s->u.ret.val) {
            if (!resolve_expr(S, s->u.ret.val)) return 0;
            if (!expr_is_value_ok(S, s->u.ret.val, "return value must be a value")) return 0;
            if (!type_eq(&s->u.ret.val->ty, &fn->ret)) {
              sema_err(S, s->line, "return type mismatch");
              return 0;
            }
          }
        }
        break;
      case OL_ST_EXPR:
        if (!resolve_expr(S, s->u.expr)) return 0;
        if (!expr_is_value_ok(S, s->u.expr, "expression statement must be a value (not a bare reference)")) return 0;
        break;
      default:
        sema_err(S, s->line, "bad stmt");
        return 0;
    }
    s = s->next;
  }
  return 1;
}

int ol_sema_check(OlProgram *p, char *err, size_t err_len) {
  SemaCtx S;
  size_t i, k;
  memset(&S, 0, sizeof(S));
  S.prog = p;

  for (i = 0; i < p->extern_count; ++i) {
    if (!type_is_valid(p, &p->externs[i].ret)) {
      snprintf(err, err_len, "invalid extern return type");
      return 0;
    }
    for (k = 0; k < p->externs[i].param_count; ++k) {
      if (!type_is_valid(p, &p->externs[i].params[k].ty) || p->externs[i].params[k].ty.kind == OL_TY_VOID) {
        snprintf(err, err_len, "invalid extern param type");
        return 0;
      }
    }
  }

  for (i = 0; i < p->extern_count; ++i) {
    for (k = i + 1; k < p->extern_count; ++k) {
      if (strcmp(p->externs[i].name, p->externs[k].name) == 0) {
        snprintf(err, err_len, "duplicate extern declaration: %s", p->externs[i].name);
        return 0;
      }
    }
  }

  for (i = 0; i < p->func_count; ++i) {
    for (k = i + 1; k < p->func_count; ++k) {
      if (strcmp(p->funcs[i].name, p->funcs[k].name) == 0) {
        snprintf(err, err_len, "duplicate function name: %s", p->funcs[i].name);
        return 0;
      }
    }
  }

  for (i = 0; i < p->func_count; ++i) {
    int exi = ol_program_find_extern(p, p->funcs[i].name);
    if (exi >= 0) {
      if (!p->funcs[i].is_export) {
        snprintf(err, err_len, "local fn %s conflicts with extern declaration (use only one translation-unit name)", p->funcs[i].name);
        return 0;
      }
      if (!signatures_match(&p->externs[(size_t)exi], &p->funcs[i])) {
        snprintf(err, err_len, "exported fn %s does not match prior extern fn declaration", p->funcs[i].name);
        return 0;
      }
    }
  }

  for (i = 0; i < p->global_count; ++i) {
    size_t bi;
    for (bi = 0; bi < p->globals[i].binding_count; ++bi) {
      size_t j, bj;
      const char *na = p->globals[i].bindings[bi].name;
      for (j = i; j < p->global_count; ++j) {
        size_t bstart = (j == i) ? (bi + 1) : 0;
        for (bj = bstart; bj < p->globals[j].binding_count; ++bj) {
          if (strcmp(p->globals[j].bindings[bj].name, na) == 0) {
            snprintf(err, err_len, "duplicate global: %s", na);
            return 0;
          }
        }
      }
      if (find_func(p, na)) {
        snprintf(err, err_len, "global name conflicts with function: %s", na);
        return 0;
      }
      if (ol_program_find_extern(p, na) >= 0) {
        snprintf(err, err_len, "global name conflicts with extern: %s", na);
        return 0;
      }
    }
    if (!validate_global_def(p, &p->globals[i], err, err_len)) return 0;
  }

  {
    uint32_t ph = hash_path_djb2(p->source_path[0] ? p->source_path : "unknown");
    for (i = 0; i < p->func_count; ++i) {
      if (p->funcs[i].is_export) {
        if (snprintf(p->funcs[i].link_name, sizeof(p->funcs[i].link_name), "%s", p->funcs[i].name) >= (int)sizeof(p->funcs[i].link_name)) {
          snprintf(err, err_len, "exported function name too long");
          return 0;
        }
      } else {
        if (snprintf(p->funcs[i].link_name, sizeof(p->funcs[i].link_name), "__ol_L_%08x_%s", (unsigned)ph, p->funcs[i].name) >= (int)sizeof(p->funcs[i].link_name)) {
          snprintf(err, err_len, "local link name too long");
          return 0;
        }
      }
    }
  }

  for (i = 0; i < p->global_count; ++i) {
    size_t j;
    int is_agg = 0;
    enter_scope(&S);
    for (j = 0; j < (size_t)i; ++j) {
      if (!bind_global_def(&S, p, &p->globals[j])) {
        snprintf(err, err_len, "global bind oom");
        exit_scope(&S);
        return 0;
      }
    }
    if (p->globals[i].binding_count == 1)
      typedef_is_aggregate(p, &p->globals[i].bindings[0].ty, &is_agg);
    if (p->globals[i].ref_expr && p->globals[i].ref_expr->kind == OL_EX_ALLOC &&
        p->globals[i].ref_expr->u.alloc_.init) {
      OlExpr *ginit = p->globals[i].ref_expr->u.alloc_.init;
      uint32_t gbw = p->globals[i].ref_expr->u.alloc_.bitwidth;
      if (!resolve_expr(&S, ginit)) {
        snprintf(err, err_len, "%s", S.err[0] ? S.err : "global init failed");
        exit_scope(&S);
        return 0;
      }
      if (is_agg) {
        if (!type_eq(&ginit->ty, &p->globals[i].bindings[0].ty)) {
          snprintf(err, err_len, "global init type mismatch");
          exit_scope(&S);
          return 0;
        }
      } else {
        if (ty_size_bytes(p, &ginit->ty) * 8u != gbw) {
          snprintf(err, err_len, "global initializer bit width must match bitwidth");
          exit_scope(&S);
          return 0;
        }
        if (p->globals[i].binding_count == 1 &&
            !type_eq(&ginit->ty, &p->globals[i].bindings[0].ty)) {
          snprintf(err, err_len, "global init type mismatch");
          exit_scope(&S);
          return 0;
        }
      }
    }
    exit_scope(&S);
  }

  for (i = 0; i < p->func_count; ++i) {
    if (!type_is_valid(p, &p->funcs[i].ret)) {
      snprintf(err, err_len, "invalid function return type");
      return 0;
    }
    enter_scope(&S);
    for (k = 0; k < p->global_count; ++k) {
      if (!bind_global_def(&S, p, &p->globals[k])) {
        snprintf(err, err_len, "duplicate global name vs function scope");
        exit_scope(&S);
        return 0;
      }
    }
    enter_scope(&S);
    for (k = 0; k < p->funcs[i].param_count; ++k) {
      if (!type_is_valid(p, &p->funcs[i].params[k].ty) || p->funcs[i].params[k].ty.kind == OL_TY_VOID) {
        snprintf(err, err_len, "invalid function param type");
        exit_scope(&S);
        exit_scope(&S);
        return 0;
      }
      if (!bind_sym(&S, p->funcs[i].params[k].name, &p->funcs[i].params[k].ty, 0, 0)) {
        snprintf(err, err_len, "duplicate param or oom");
        exit_scope(&S);
        exit_scope(&S);
        return 0;
      }
    }
    if (!check_stmt(&S, &p->funcs[i], p->funcs[i].body)) {
      snprintf(err, err_len, "%s", S.err[0] ? S.err : "sema failed");
      exit_scope(&S);
      exit_scope(&S);
      return 0;
    }
    if (p->funcs[i].ret.kind != OL_TY_VOID && !func_body_returns(p->funcs[i].body)) {
      snprintf(err, err_len, "non-void function must return on all paths");
      exit_scope(&S);
      exit_scope(&S);
      return 0;
    }
    exit_scope(&S);
    exit_scope(&S);
  }
  return 1;
}
