#include "ir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_expr(OlExpr *e) {
  size_t i;
  if (!e) return;
  switch (e->kind) {
    case OL_EX_BINARY:
      free_expr(e->u.binary.left);
      free_expr(e->u.binary.right);
      break;
    case OL_EX_CALL:
      for (i = 0; i < e->u.call.arg_count; ++i) free_expr(e->u.call.args[i]);
      free(e->u.call.args);
      break;
    case OL_EX_NEG:
      free_expr(e->u.neg.inner);
      break;
    case OL_EX_NOT:
      free_expr(e->u.not_.inner);
      break;
    case OL_EX_CAST:
      free_expr(e->u.cast_.inner);
      break;
    case OL_EX_REF_BIND:
      free_expr(e->u.ref_bind.inner);
      break;
    case OL_EX_FIND:
      free_expr(e->u.find_.inner);
      break;
    case OL_EX_LOAD:
      free_expr(e->u.load_.inner);
      break;
    case OL_EX_ALLOC:
      free_expr(e->u.alloc_.init);
      break;
    case OL_EX_BNOT:
      free_expr(e->u.bnot.inner);
      break;
    case OL_EX_FIELD:
      free_expr(e->u.field.obj);
      break;
    case OL_EX_INDEX:
      free_expr(e->u.index_.arr);
      free_expr(e->u.index_.index_expr);
      break;
    case OL_EX_ADDR:
      free_expr(e->u.addr.inner);
      break;
    default:
      break;
  }
  free(e);
}

static void free_stmt(OlStmt *s) {
  OlStmt *n;
  while (s) {
    n = s->next;
    switch (s->kind) {
      case OL_ST_BLOCK: {
        OlStmt *c = s->u.block.first;
        free_stmt(c);
        break;
      }
      case OL_ST_LET:
        free_expr(s->u.let_.ref_expr);
        break;
      case OL_ST_STORE:
        free_expr(s->u.store_.target);
        free_expr(s->u.store_.val);
        break;
      case OL_ST_IF:
        free_expr(s->u.if_.cond);
        free_stmt(s->u.if_.then_arm);
        free_stmt(s->u.if_.else_arm);
        break;
      case OL_ST_WHILE:
        free_expr(s->u.while_.cond);
        free_stmt(s->u.while_.body);
        break;
      case OL_ST_RETURN:
        free_expr(s->u.ret.val);
        break;
      case OL_ST_EXPR:
        free_expr(s->u.expr);
        break;
      default:
        break;
    }
    free(s);
    s = n;
  }
}

void ol_program_init(OlProgram *p) {
  memset(p, 0, sizeof(*p));
}

void ol_program_free(OlProgram *p) {
  size_t i;
  for (i = 0; i < p->extern_count; ++i) free(p->externs[i].params);
  free(p->externs);
  for (i = 0; i < p->typedef_count; ++i) free(p->typedefs[i].fields);
  free(p->typedefs);
  for (i = 0; i < p->global_count; ++i) free_expr(p->globals[i].ref_expr);
  free(p->globals);
  for (i = 0; i < p->func_count; ++i) {
    free(p->funcs[i].params);
    free_stmt(p->funcs[i].body);
  }
  free(p->funcs);
  for (i = 0; i < p->string_count; ++i) free(p->strings[i].bytes);
  free(p->strings);
  memset(p, 0, sizeof(*p));
}

int ol_program_find_extern(const OlProgram *p, const char *name) {
  size_t i;
  for (i = 0; i < p->extern_count; ++i) {
    if (strcmp(p->externs[i].name, name) == 0) return (int)i;
  }
  return -1;
}

const OlExternDecl *ol_program_get_extern(const OlProgram *p, const char *name) {
  int i = ol_program_find_extern(p, name);
  if (i < 0) return NULL;
  return &p->externs[(size_t)i];
}

int ol_program_find_global(const OlProgram *p, const char *name) {
  size_t i, j;
  for (i = 0; i < p->global_count; ++i) {
    for (j = 0; j < p->globals[i].binding_count; ++j) {
      if (strcmp(p->globals[i].bindings[j].name, name) == 0) return (int)i;
    }
  }
  return -1;
}

int ol_program_find_typedef(const OlProgram *p, const char *name) {
  size_t i;
  for (i = 0; i < p->typedef_count; ++i) {
    if (strcmp(p->typedefs[i].name, name) == 0) return (int)i;
  }
  return -1;
}

int ol_program_add_typedef_struct(OlProgram *p, const char *name) {
  OlTypeDef *next;
  if (ol_program_find_typedef(p, name) >= 0) return -1;
  next = (OlTypeDef *)realloc(p->typedefs, (p->typedef_count + 1) * sizeof(OlTypeDef));
  if (!next) return -1;
  p->typedefs = next;
  memset(&p->typedefs[p->typedef_count], 0, sizeof(OlTypeDef));
  p->typedefs[p->typedef_count].kind = OL_TYPEDEF_STRUCT;
  snprintf(p->typedefs[p->typedef_count].name, sizeof(p->typedefs[p->typedef_count].name), "%s", name);
  p->typedefs[p->typedef_count].size_bytes = 0;
  p->typedef_count++;
  return (int)(p->typedef_count - 1);
}

int ol_program_add_typedef_struct_field(OlProgram *p, size_t type_idx, const char *field_name, const char *field_type) {
  OlTypeDef *t;
  OlTypeDefStructField *nf;
  uint32_t sz = 8;
  if (type_idx >= p->typedef_count) return 0;
  t = &p->typedefs[type_idx];
  if (t->kind != OL_TYPEDEF_STRUCT) return 0;
  if (strcmp(field_type, "bool") == 0 || strcmp(field_type, "u8") == 0 || strcmp(field_type, "i8") == 0 || strcmp(field_type, "b8") == 0)
    sz = 1;
  else if (strcmp(field_type, "u16") == 0 || strcmp(field_type, "i16") == 0 || strcmp(field_type, "f16") == 0 || strcmp(field_type, "b16") == 0)
    sz = 2;
  else if (strcmp(field_type, "i32") == 0 || strcmp(field_type, "u32") == 0 || strcmp(field_type, "f32") == 0 || strcmp(field_type, "b32") == 0)
    sz = 4;
  else if (strcmp(field_type, "i64") == 0 || strcmp(field_type, "u64") == 0 || strcmp(field_type, "f64") == 0 || strcmp(field_type, "b64") == 0 || strcmp(field_type, "ptr") == 0)
    sz = 8;
  else if (ol_program_find_typedef(p, field_type) >= 0) {
    int k = ol_program_find_typedef(p, field_type);
    sz = p->typedefs[k].size_bytes;
    if (sz == 0) sz = 8;
  }
  nf = (OlTypeDefStructField *)realloc(t->fields, (t->field_count + 1) * sizeof(OlTypeDefStructField));
  if (!nf) return 0;
  t->fields = nf;
  memset(&t->fields[t->field_count], 0, sizeof(OlTypeDefStructField));
  snprintf(t->fields[t->field_count].name, sizeof(t->fields[t->field_count].name), "%s", field_name);
  snprintf(t->fields[t->field_count].type_name, sizeof(t->fields[t->field_count].type_name), "%s", field_type);
  t->fields[t->field_count].offset = t->size_bytes;
  t->size_bytes += sz;
  t->field_count++;
  return 1;
}

int ol_program_add_typedef_array(OlProgram *p, const char *name, const char *elem_type, uint32_t count) {
  OlTypeDef *next;
  uint32_t elem_sz = 8;
  if (ol_program_find_typedef(p, name) >= 0) return 0;
  if (strcmp(elem_type, "bool") == 0 || strcmp(elem_type, "u8") == 0 || strcmp(elem_type, "i8") == 0 || strcmp(elem_type, "b8") == 0)
    elem_sz = 1;
  else if (strcmp(elem_type, "u16") == 0 || strcmp(elem_type, "i16") == 0 || strcmp(elem_type, "f16") == 0 || strcmp(elem_type, "b16") == 0)
    elem_sz = 2;
  else if (strcmp(elem_type, "i32") == 0 || strcmp(elem_type, "u32") == 0 || strcmp(elem_type, "f32") == 0 || strcmp(elem_type, "b32") == 0)
    elem_sz = 4;
  else if (strcmp(elem_type, "i64") == 0 || strcmp(elem_type, "u64") == 0 || strcmp(elem_type, "f64") == 0 || strcmp(elem_type, "b64") == 0 || strcmp(elem_type, "ptr") == 0)
    elem_sz = 8;
  else if (ol_program_find_typedef(p, elem_type) >= 0) {
    int k = ol_program_find_typedef(p, elem_type);
    elem_sz = p->typedefs[k].size_bytes;
    if (elem_sz == 0) elem_sz = 8;
  }
  next = (OlTypeDef *)realloc(p->typedefs, (p->typedef_count + 1) * sizeof(OlTypeDef));
  if (!next) return 0;
  p->typedefs = next;
  memset(&p->typedefs[p->typedef_count], 0, sizeof(OlTypeDef));
  p->typedefs[p->typedef_count].kind = OL_TYPEDEF_ARRAY;
  snprintf(p->typedefs[p->typedef_count].name, sizeof(p->typedefs[p->typedef_count].name), "%s", name);
  snprintf(p->typedefs[p->typedef_count].elem_type, sizeof(p->typedefs[p->typedef_count].elem_type), "%s", elem_type);
  p->typedefs[p->typedef_count].elem_count = count;
  p->typedefs[p->typedef_count].size_bytes = elem_sz * count;
  p->typedef_count++;
  return 1;
}
