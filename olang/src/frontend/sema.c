#include "sema.h"

#include "ast.h"

#include <stdint.h>
#include <stdio.h>
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
  char ptrbind_sym[128];
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
  return 1;
}

static void type_copy(OlTypeRef *dst, const OlTypeRef *src) { *dst = *src; }

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

static int can_explicit_cast(const OlTypeRef *from, const OlTypeRef *to) {
  if (type_eq(from, to)) return 0; /* same type same width: use reinterpret */
  /* uN -> uN */
  if (type_is_unsigned_int(from) && type_is_unsigned_int(to)) return 1;
  /* iN -> iN */
  if (type_is_signed_int(from) && type_is_signed_int(to)) return 1;
  /* bN -> bN */
  if (type_is_binary(from) && type_is_binary(to)) return 1;
  /* fN -> fN */
  if (type_is_float(from) && type_is_float(to)) return 1;
  /* fN <-> iN */
  if (type_is_float(from) && type_is_signed_int(to)) return 1;
  if (type_is_signed_int(from) && type_is_float(to)) return 1;
  /* fN <-> uN */
  if (type_is_float(from) && type_is_unsigned_int(to)) return 1;
  if (type_is_unsigned_int(from) && type_is_float(to)) return 1;
  /* bool -> uN/iN */
  if (from->kind == OL_TY_BOOL && (type_is_unsigned_int(to) || type_is_signed_int(to))) return 1;
  /* uN/iN -> bool */
  if ((type_is_unsigned_int(from) || type_is_signed_int(from)) && to->kind == OL_TY_BOOL) return 1;
  return 0;
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
    case OL_TY_ALIAS:
      k = ol_program_find_typedef(p, t->alias_name);
      if (k < 0) return 0u;
      return p->typedefs[k].size_bytes;
  }
  return 0u;
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

static int bind_sym(SemaCtx *S, const char *name, const OlTypeRef *ty, const char *ptrbind) {
  if (S->sym_count >= MAX_SYM) return 0;
  if (exists_in_current_scope(S, name)) return 0;
  snprintf(S->syms[S->sym_count].name, sizeof(S->syms[S->sym_count].name), "%s", name);
  S->syms[S->sym_count].ty = *ty;
  S->syms[S->sym_count].ptrbind_sym[0] = '\0';
  if (ptrbind && ptrbind[0]) {
    snprintf(S->syms[S->sym_count].ptrbind_sym, sizeof(S->syms[S->sym_count].ptrbind_sym), "%s", ptrbind);
  }
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
  if (gi < 0) return NULL;
  return &p->globals[(size_t)gi].ty;
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
      type_copy(&e->ty, &sl->ty);
      return 1;
    }
    case OL_EX_NEG:
      if (!resolve_expr(S, e->u.neg.inner)) return 0;
      if (!type_is_int(&e->u.neg.inner->ty) && !type_is_float(&e->u.neg.inner->ty) && !type_is_binary(&e->u.neg.inner->ty)) {
        sema_err(S, e->line, "negation requires int, float, or binary operand");
        return 0;
      }
      type_copy(&e->ty, &e->u.neg.inner->ty);
      return 1;
    case OL_EX_NOT:
      if (!resolve_expr(S, e->u.not_.inner)) return 0;
      if (e->u.not_.inner->ty.kind != OL_TY_BOOL) {
        sema_err(S, e->line, "! requires bool operand");
        return 0;
      }
      e->ty.kind = OL_TY_BOOL;
      return 1;
    case OL_EX_BNOT:
      if (!resolve_expr(S, e->u.bnot.inner)) return 0;
      if (!type_is_binary(&e->u.bnot.inner->ty)) {
        sema_err(S, e->line, "~ requires binary type operand");
        return 0;
      }
      type_copy(&e->ty, &e->u.bnot.inner->ty);
      return 1;
    case OL_EX_BINARY: {
      OlBinOp op = e->u.binary.op;
      if (!resolve_expr(S, e->u.binary.left) || !resolve_expr(S, e->u.binary.right)) return 0;
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
      if (!resolve_expr(S, e->u.cast_.inner)) return 0;
      if (!type_is_valid(S->prog, &e->u.cast_.to) || e->u.cast_.to.kind == OL_TY_VOID) {
        sema_err(S, e->line, "invalid cast target type");
        return 0;
      }
      if (!can_explicit_cast(&e->u.cast_.inner->ty, &e->u.cast_.to)) {
        sema_err(S, e->line, "invalid cast (use reinterpret for same-size type rebinding)");
        return 0;
      }
      e->ty = e->u.cast_.to;
      return 1;
    }
    case OL_EX_REINTERPRET: {
      if (!resolve_expr(S, e->u.reinterpret_.inner)) return 0;
      if (!type_is_valid(S->prog, &e->u.reinterpret_.to) || e->u.reinterpret_.to.kind == OL_TY_VOID) {
        sema_err(S, e->line, "invalid reinterpret target type");
        return 0;
      }
      if (e->u.reinterpret_.inner->ty.kind == OL_TY_BOOL || e->u.reinterpret_.to.kind == OL_TY_BOOL) {
        sema_err(S, e->line, "bool cannot participate in reinterpret");
        return 0;
      }
      if (e->u.reinterpret_.inner->ty.kind == OL_TY_ALIAS || e->u.reinterpret_.to.kind == OL_TY_ALIAS) {
        sema_err(S, e->line, "reinterpret not supported for aggregate types");
        return 0;
      }
      {
        uint32_t from_sz = ty_size_bytes(S->prog, &e->u.reinterpret_.inner->ty);
        uint32_t to_sz = ty_size_bytes(S->prog, &e->u.reinterpret_.to);
        if (from_sz != to_sz || from_sz == 0) {
          sema_err(S, e->line, "reinterpret requires same bit width");
          return 0;
        }
      }
      e->ty = e->u.reinterpret_.to;
      return 1;
    }
    case OL_EX_ADDR: {
      SymSlot *sl = lookup_sym(S, e->u.addr.name);
      if (!sl) {
        sema_err(S, e->line, "addr of unknown");
        return 0;
      }
      (void)sl;
      e->ty.kind = OL_TY_PTR;
      return 1;
    }
    case OL_EX_LOAD: {
      if (!resolve_expr(S, e->u.load.ptr)) return 0;
      if (e->u.load.ptr->ty.kind != OL_TY_PTR) {
        sema_err(S, e->line, "load needs ptr");
        return 0;
      }
      if (!type_is_valid(S->prog, &e->u.load.elem_ty) || e->u.load.elem_ty.kind == OL_TY_VOID) {
        sema_err(S, e->line, "invalid load element type");
        return 0;
      }
      type_copy(&e->ty, &e->u.load.elem_ty);
      return 1;
    }
    case OL_EX_STORE: {
      if (!resolve_expr(S, e->u.store.ptr) || !resolve_expr(S, e->u.store.val)) return 0;
      if (e->u.store.ptr->ty.kind != OL_TY_PTR) {
        sema_err(S, e->line, "store needs ptr");
        return 0;
      }
      if (!type_is_valid(S->prog, &e->u.store.elem_ty) || e->u.store.elem_ty.kind == OL_TY_VOID) {
        sema_err(S, e->line, "invalid store element type");
        return 0;
      }
      if (!type_eq(&e->u.store.val->ty, &e->u.store.elem_ty)) {
        sema_err(S, e->line, "store value type mismatch");
        return 0;
      }
      e->ty.kind = OL_TY_VOID;
      return 1;
    }
    case OL_EX_FIELD: {
      OlTypeRef st;
      OlTypeDefKind kind;
      if (!resolve_expr(S, e->u.field.obj)) return 0;
      type_copy(&st, &e->u.field.obj->ty);
      if (!resolve_alias_kind(S->prog, &st, &kind) || kind != OL_TYPEDEF_STRUCT) {
        sema_err(S, e->line, "field on non-struct");
        return 0;
      }
      if (!resolve_field_type(S->prog, st.alias_name, e->u.field.field, &e->ty)) {
        sema_err(S, e->line, "unknown field");
        return 0;
      }
      return 1;
    }
    case OL_EX_INDEX: {
      OlTypeRef at;
      OlTypeDefKind kind;
      int ti;
      if (!resolve_expr(S, e->u.index_.arr)) return 0;
      if (!resolve_expr(S, e->u.index_.index_expr)) return 0;
      if (!type_is_int(&e->u.index_.index_expr->ty)) {
        sema_err(S, e->line, "array index must be integer");
        return 0;
      }
      type_copy(&at, &e->u.index_.arr->ty);
      if (!resolve_alias_kind(S->prog, &at, &kind) || kind != OL_TYPEDEF_ARRAY) {
        sema_err(S, e->line, "index on non-array");
        return 0;
      }
      ti = ol_program_find_typedef(S->prog, at.alias_name);
      if (ti < 0) {
        sema_err(S, e->line, "unknown array type");
        return 0;
      }
      if (!resolve_array_elem_type(S->prog, at.alias_name, &e->ty)) {
        sema_err(S, e->line, "bad array elem type");
        return 0;
      }
      return 1;
    }
    default:
      sema_err(S, e->line, "bad expr");
      return 0;
  }
}

static int check_stmt(SemaCtx *S, const OlFuncDef *fn, OlStmt *s);

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
        int is_aggregate = 0;
        if (lookup_global_type(S->prog, s->u.let_.name)) {
          sema_err(S, s->line, "let name shadows global");
          return 0;
        }
        if (!type_is_valid(S->prog, &s->u.let_.ty) || s->u.let_.ty.kind == OL_TY_VOID) {
          sema_err(S, s->line, "invalid let type");
          return 0;
        }
        /* Check if type is aggregate (struct or array) by looking up typedef */
        if (s->u.let_.ty.kind == OL_TY_ALIAS) {
          int tdi = ol_program_find_typedef(S->prog, s->u.let_.ty.alias_name);
          if (tdi >= 0) {
            OlTypeDefKind tdk = S->prog->typedefs[(size_t)tdi].kind;
            if (tdk == OL_TYPEDEF_STRUCT || tdk == OL_TYPEDEF_ARRAY) {
              is_aggregate = 1;
            }
          }
        }
        /* Aggregate types (struct/array) don't require initializer regardless of size */
        if (is_aggregate) {
          /* Aggregate: no init required, bind the type */
          if (!bind_sym(S, s->u.let_.name, &s->u.let_.ty, NULL)) {
            sema_err(S, s->line, "duplicate let or oom");
            return 0;
          }
        } else {
          /* Scalar: must have init expression */
          if (!s->u.let_.init) {
            sema_err(S, s->line, "scalar let requires initializer");
            return 0;
          }
          if (!resolve_expr(S, s->u.let_.init)) return 0;
          if (!type_eq(&s->u.let_.init->ty, &s->u.let_.ty)) {
            sema_err(S, s->line, "let type mismatch");
            return 0;
          }
          if (!bind_sym(S, s->u.let_.name, &s->u.let_.ty, NULL)) {
            sema_err(S, s->line, "duplicate let or oom");
            return 0;
          }
        }
        break;
      }
      case OL_ST_ASSIGN: {
        OlTypeRef lhs_ty;
        OlExpr *lhs = s->u.assign_.lhs;
        const OlTypeRef *lhs_ty_ptr = NULL;
        memset(&lhs_ty, 0, sizeof(lhs_ty));
        if (!lhs) { sema_err(S, s->line, "assign has no left side"); return 0; }
        /* Resolve left value based on its kind */
        if (lhs->kind == OL_EX_VAR) {
          SymSlot *sl = lookup_sym(S, lhs->u.var_name);
          int gi = ol_program_find_global(S->prog, lhs->u.var_name);
          if (sl && gi >= 0) { sema_err(S, s->line, "assign target ambiguous"); return 0; }
          if (sl) lhs_ty_ptr = &sl->ty;
          if (gi >= 0) {
            if (global_storage_is_rodata(&S->prog->globals[(size_t)gi])) {
              sema_err(S, s->line, "cannot assign to @rodata global");
              return 0;
            }
            lhs_ty_ptr = &S->prog->globals[(size_t)gi].ty;
          }
          if (!lhs_ty_ptr) { sema_err(S, s->line, "assign to unknown name"); return 0; }
          lhs_ty = *lhs_ty_ptr;
        } else if (lhs->kind == OL_EX_INDEX) {
          /* Array index: arr[idx] = value */
          if (!resolve_expr(S, lhs->u.index_.arr)) return 0;
          if (!resolve_expr(S, lhs->u.index_.index_expr)) return 0;
          if (!type_is_int(&lhs->u.index_.index_expr->ty)) {
            sema_err(S, lhs->u.index_.index_expr->line, "array index must be integer");
            return 0;
          }
          {
            OlTypeRef at;
            OlTypeDefKind kind;
            type_copy(&at, &lhs->u.index_.arr->ty);
            if (!resolve_alias_kind(S->prog, &at, &kind) || kind != OL_TYPEDEF_ARRAY) {
              sema_err(S, lhs->line, "index on non-array");
              return 0;
            }
            if (!resolve_array_elem_type(S->prog, at.alias_name, &lhs_ty)) {
              sema_err(S, lhs->line, "bad array elem type");
              return 0;
            }
          }
        } else if (lhs->kind == OL_EX_FIELD) {
          /* Struct field assignment: obj.field = value */
          OlTypeRef st;
          OlTypeDefKind kind;
          if (!resolve_expr(S, lhs->u.field.obj)) return 0;
          type_copy(&st, &lhs->u.field.obj->ty);
          if (!resolve_alias_kind(S->prog, &st, &kind) || kind != OL_TYPEDEF_STRUCT) {
            sema_err(S, lhs->line, "field assign on non-struct");
            return 0;
          }
          if (!resolve_field_type(S->prog, st.alias_name, lhs->u.field.field, &lhs_ty)) {
            sema_err(S, lhs->line, "unknown field for assignment");
            return 0;
          }
        } else {
          sema_err(S, s->line, "invalid left value for assignment");
          return 0;
        }
        if (!resolve_expr(S, s->u.assign_.rhs)) return 0;
        if (!type_eq(&s->u.assign_.rhs->ty, &lhs_ty)) {
          sema_err(S, s->line, "assign type mismatch");
          return 0;
        }
        break;
      }
      case OL_ST_IF:
        if (!resolve_expr(S, s->u.if_.cond)) return 0;
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
            if (!type_eq(&s->u.ret.val->ty, &fn->ret)) {
              sema_err(S, s->line, "return type mismatch");
              return 0;
            }
          }
        }
        break;
      case OL_ST_EXPR:
        if (!resolve_expr(S, s->u.expr)) return 0;
        break;
      case OL_ST_PTRBIND: {
        OlTypeRef ptrty;
        if (!type_is_valid(S->prog, &s->u.ptrbind.ty) || s->u.ptrbind.ty.kind == OL_TY_VOID) {
          sema_err(S, s->line, "ptrbind typed view must be concrete type");
          return 0;
        }
        if (ol_program_find_extern(S->prog, s->u.ptrbind.symbol) < 0 && !find_func(S->prog, s->u.ptrbind.symbol)) {
          sema_err(S, s->line, "ptrbind unknown symbol");
          return 0;
        }
        memset(&ptrty, 0, sizeof(ptrty));
        ptrty.kind = OL_TY_PTR;
        if (!bind_sym(S, s->u.ptrbind.bind, &ptrty, s->u.ptrbind.symbol)) {
          sema_err(S, s->line, "duplicate ptrbind or oom");
          return 0;
        }
        break;
      }
      case OL_ST_DEREF: {
        SymSlot *sl = lookup_sym(S, s->u.deref.bind);
        if (!sl) {
          sema_err(S, s->line, "deref unknown bind");
          return 0;
        }
        if (sl->ty.kind != OL_TY_PTR) {
          sema_err(S, s->line, "deref source must be ptr");
          return 0;
        }
        if (!sl->ptrbind_sym[0]) {
          sema_err(S, s->line, "deref requires ptrbind-backed symbol");
          return 0;
        }
        if (!type_is_valid(S->prog, &s->u.deref.ty) || s->u.deref.ty.kind == OL_TY_VOID) {
          sema_err(S, s->line, "deref target type invalid");
          return 0;
        }
        {
          uint32_t dz = ty_size_bytes(S->prog, &s->u.deref.ty);
          if (dz == 0u || dz > 8u) {
            sema_err(S, s->line, "deref type must be 4 or 8 bytes");
            return 0;
          }
        }
        /* After deref codegen, slot holds loaded value of type T. */
        sl->ty = s->u.deref.ty;
        break;
      }
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
    for (k = i + 1; k < p->global_count; ++k) {
      if (strcmp(p->globals[i].name, p->globals[k].name) == 0) {
        snprintf(err, err_len, "duplicate global: %s", p->globals[i].name);
        return 0;
      }
    }
    if (find_func(p, p->globals[i].name)) {
      snprintf(err, err_len, "global name conflicts with function: %s", p->globals[i].name);
      return 0;
    }
    if (ol_program_find_extern(p, p->globals[i].name) >= 0) {
      snprintf(err, err_len, "global name conflicts with extern: %s", p->globals[i].name);
      return 0;
    }
    if (!type_is_valid(p, &p->globals[i].ty) || p->globals[i].ty.kind == OL_TY_VOID) {
      snprintf(err, err_len, "invalid global type");
      return 0;
    }
    if (p->globals[i].section == OL_GSEC_BSS && p->globals[i].init) {
      snprintf(err, err_len, "@bss global cannot have initializer");
      return 0;
    }
    if (global_storage_is_rodata(&p->globals[i])) {
      if (!p->globals[i].init) {
        snprintf(err, err_len, "@rodata global requires initializer");
        return 0;
      }
      if (!init_expr_is_const(p->globals[i].init)) {
        snprintf(err, err_len, "@rodata initializer must be constant");
        return 0;
      }
    }
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
    enter_scope(&S);
    for (j = 0; j < (size_t)i; ++j) {
      if (!bind_sym(&S, p->globals[j].name, &p->globals[j].ty, NULL)) {
        snprintf(err, err_len, "global bind oom");
        exit_scope(&S);
        return 0;
      }
    }
    if (p->globals[i].init) {
      if (!resolve_expr(&S, p->globals[i].init)) {
        snprintf(err, err_len, "%s", S.err[0] ? S.err : "global init failed");
        exit_scope(&S);
        return 0;
      }
      if (!type_eq(&p->globals[i].init->ty, &p->globals[i].ty)) {
        snprintf(err, err_len, "global init type mismatch");
        exit_scope(&S);
        return 0;
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
      if (!bind_sym(&S, p->globals[k].name, &p->globals[k].ty, NULL)) {
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
      if (!bind_sym(&S, p->funcs[i].params[k].name, &p->funcs[i].params[k].ty, NULL)) {
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
