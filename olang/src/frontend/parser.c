#define _POSIX_C_SOURCE 200809L

#include "parser.h"

#include "ast.h"
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ParseCtx {
  OlProgram *prog;
  OlLexer *L;
  char *err;
  size_t err_len;
} ParseCtx;

static void free_expr(OlExpr *e);
static void free_stmt(OlStmt *s);

static void errf(ParseCtx *C, const char *msg) {
  const char *m = msg;
  if (!C->err || C->err_len == 0) return;
  if (C->L->lexer_error && C->L->errmsg[0]) m = C->L->errmsg;
  snprintf(C->err, C->err_len, "%s:%d: %s", C->L->path, C->L->line, m);
}

static int lex_next(ParseCtx *C) {
  ol_lexer_next(C->L);
  if (C->L->lexer_error) {
    errf(C, C->L->errmsg);
    return 0;
  }
  return 1;
}

static OlExpr *new_expr(OlExprKind k, int line) {
  OlExpr *e = (OlExpr *)calloc(1, sizeof(OlExpr));
  if (e) {
    e->kind = k;
    e->line = line;
    e->ty.kind = OL_TY_VOID;
  }
  return e;
}

static OlStmt *new_stmt(OlStmtKind k, int line) {
  OlStmt *s = (OlStmt *)calloc(1, sizeof(OlStmt));
  if (s) {
    s->kind = k;
    s->line = line;
  }
  return s;
}

static int expect(ParseCtx *C, OlTok t) {
  if (C->L->tok != t) {
    errf(C, "unexpected token");
    return 0;
  }
  return lex_next(C);
}

static int is_builtin_type_tok(OlTok t) {
  return t == TOK_KW_VOID || t == TOK_KW_BOOL || t == TOK_KW_U8 || t == TOK_KW_I32 || t == TOK_KW_U32 || t == TOK_KW_I64 ||
         t == TOK_KW_U64 || t == TOK_KW_PTR;
}

static int parse_type_ref(ParseCtx *C, OlTypeRef *out) {
  if (C->L->tok == TOK_KW_VOID) {
    if (!lex_next(C)) return 0;
    out->kind = OL_TY_VOID;
    return 1;
  }
  if (C->L->tok == TOK_KW_BOOL) {
    if (!lex_next(C)) return 0;
    out->kind = OL_TY_BOOL;
    return 1;
  }
  if (C->L->tok == TOK_KW_U8) {
    if (!lex_next(C)) return 0;
    out->kind = OL_TY_U8;
    return 1;
  }
  if (C->L->tok == TOK_KW_I32) {
    if (!lex_next(C)) return 0;
    out->kind = OL_TY_I32;
    return 1;
  }
  if (C->L->tok == TOK_KW_U32) {
    if (!lex_next(C)) return 0;
    out->kind = OL_TY_U32;
    return 1;
  }
  if (C->L->tok == TOK_KW_I64) {
    if (!lex_next(C)) return 0;
    out->kind = OL_TY_I64;
    return 1;
  }
  if (C->L->tok == TOK_KW_U64) {
    if (!lex_next(C)) return 0;
    out->kind = OL_TY_U64;
    return 1;
  }
  if (C->L->tok == TOK_KW_PTR) {
    if (!lex_next(C)) return 0;
    out->kind = OL_TY_PTR;
    return 1;
  }
  if (C->L->tok == TOK_IDENT) {
    snprintf(out->alias_name, sizeof(out->alias_name), "%s", C->L->ident);
    if (!lex_next(C)) return 0;
    out->kind = OL_TY_ALIAS;
    return 1;
  }
  errf(C, "expected type");
  return 0;
}

static int parse_type_after_lt(ParseCtx *C, OlTypeRef *out) {
  if (!expect(C, TOK_LT)) return 0;
  if (!parse_type_ref(C, out)) return 0;
  if (!expect(C, TOK_GT)) return 0;
  return 1;
}

static size_t add_string(OlProgram *p, const uint8_t *data, size_t n) {
  OlStringLit *ns;
  size_t idx = p->string_count;
  ns = (OlStringLit *)realloc(p->strings, (p->string_count + 1) * sizeof(OlStringLit));
  if (!ns) return (size_t)-1;
  p->strings = ns;
  memset(&p->strings[p->string_count], 0, sizeof(OlStringLit));
  p->strings[p->string_count].bytes = (uint8_t *)malloc(n + 1);
  if (!p->strings[p->string_count].bytes) return (size_t)-1;
  memcpy(p->strings[p->string_count].bytes, data, n);
  p->strings[p->string_count].bytes[n] = 0;
  p->strings[p->string_count].len = n;
  snprintf(p->strings[p->string_count].sym, sizeof(p->strings[p->string_count].sym), ".str%zu", p->string_count);
  p->string_count++;
  return idx;
}

static OlExpr *parse_expr(ParseCtx *C);
static OlExpr *parse_unary(ParseCtx *C);
static OlExpr *parse_mul(ParseCtx *C);
static OlExpr *parse_add(ParseCtx *C);

static OlExpr *parse_primary(ParseCtx *C) {
  int line = C->L->line;
  if (C->L->tok == TOK_INT) {
    OlExpr *e = new_expr(OL_EX_INT, line);
    if (!e) return NULL;
    e->u.int_.int_val = C->L->int_val;
    e->u.int_.int_suffix = C->L->int_suffix;
    if (!lex_next(C)) return NULL;
    return e;
  }
  if (C->L->tok == TOK_STR) {
    size_t idx;
    OlExpr *e = new_expr(OL_EX_STR, line);
    if (!e) return NULL;
    idx = add_string(C->prog, (const uint8_t *)C->L->str_val, C->L->str_len);
    if (idx == (size_t)-1) {
      errf(C, "oom string");
      free(e);
      return NULL;
    }
    e->u.str_idx = idx;
    if (!lex_next(C)) return NULL;
    return e;
  }
  if (C->L->tok == TOK_IDENT) {
    OlExpr *e = new_expr(OL_EX_VAR, line);
    if (!e) return NULL;
    snprintf(e->u.var_name, sizeof(e->u.var_name), "%s", C->L->ident);
    if (!lex_next(C)) return NULL;
    return e;
  }
  if (C->L->tok == TOK_LPAREN) {
    OlExpr *inner;
    if (!lex_next(C)) return NULL;
    inner = parse_expr(C);
    if (!inner) return NULL;
    if (!expect(C, TOK_RPAREN)) {
      free_expr(inner);
      return NULL;
    }
    return inner;
  }
  if (C->L->tok == TOK_KW_CAST) {
    OlExpr *in;
    OlExpr *e = new_expr(OL_EX_CAST, line);
    if (!e) return NULL;
    if (!lex_next(C)) return NULL;
    if (!parse_type_after_lt(C, &e->u.cast_.to)) {
      free(e);
      return NULL;
    }
    if (!expect(C, TOK_LPAREN)) {
      free(e);
      return NULL;
    }
    in = parse_expr(C);
    if (!in) {
      free(e);
      return NULL;
    }
    e->u.cast_.inner = in;
    if (!expect(C, TOK_RPAREN)) {
      free_expr(e);
      return NULL;
    }
    return e;
  }
  if (C->L->tok == TOK_KW_LOAD) {
    OlExpr *p;
    OlExpr *e = new_expr(OL_EX_LOAD, line);
    if (!e) return NULL;
    if (!lex_next(C)) return NULL;
    if (!parse_type_after_lt(C, &e->u.load.elem_ty)) {
      free(e);
      return NULL;
    }
    if (!expect(C, TOK_LPAREN)) {
      free(e);
      return NULL;
    }
    p = parse_expr(C);
    if (!p) {
      free(e);
      return NULL;
    }
    e->u.load.ptr = p;
    if (!expect(C, TOK_RPAREN)) {
      free_expr(e);
      return NULL;
    }
    return e;
  }
  if (C->L->tok == TOK_KW_STORE) {
    OlExpr *ptr, *val;
    OlExpr *e = new_expr(OL_EX_STORE, line);
    if (!e) return NULL;
    if (!lex_next(C)) return NULL;
    if (!parse_type_after_lt(C, &e->u.store.elem_ty)) {
      free(e);
      return NULL;
    }
    if (!expect(C, TOK_LPAREN)) {
      free(e);
      return NULL;
    }
    ptr = parse_expr(C);
    if (!ptr) {
      free(e);
      return NULL;
    }
    if (!expect(C, TOK_COMMA)) {
      free_expr(ptr);
      free(e);
      return NULL;
    }
    val = parse_expr(C);
    if (!val) {
      free_expr(ptr);
      free(e);
      return NULL;
    }
    e->u.store.ptr = ptr;
    e->u.store.val = val;
    if (!expect(C, TOK_RPAREN)) {
      free_expr(e);
      return NULL;
    }
    return e;
  }
  if (C->L->tok == TOK_KW_ADDR) {
    OlExpr *e = new_expr(OL_EX_ADDR, line);
    if (!e) return NULL;
    if (!lex_next(C)) return NULL;
    if (C->L->tok != TOK_IDENT) {
      errf(C, "expected ident after addr");
      free(e);
      return NULL;
    }
    snprintf(e->u.addr.name, sizeof(e->u.addr.name), "%s", C->L->ident);
    if (!lex_next(C)) return NULL;
    return e;
  }
  if (C->L->tok == TOK_KW_TRUE || C->L->tok == TOK_KW_FALSE) {
    OlExpr *e = new_expr(OL_EX_BOOL, line);
    if (!e) return NULL;
    e->u.bool_val = (C->L->tok == TOK_KW_TRUE) ? 1 : 0;
    if (!lex_next(C)) return NULL;
    return e;
  }
  if (C->L->tok == TOK_CHAR) {
    OlExpr *e = new_expr(OL_EX_CHAR, line);
    if (!e) return NULL;
    e->u.char_val = C->L->char_val;
    if (!lex_next(C)) return NULL;
    return e;
  }
  errf(C, "expected expression");
  return NULL;
}

static OlExpr *parse_postfix_chain(ParseCtx *C, OlExpr *e) {
  for (;;) {
    if (C->L->tok == TOK_LPAREN) {
      OlExpr *call = new_expr(OL_EX_CALL, e->line);
      size_t cap = 8, n = 0;
      OlExpr **args = NULL;
      if (!call) {
        free_expr(e);
        return NULL;
      }
      if (e->kind != OL_EX_VAR) {
        errf(C, "call target must be identifier");
        free_expr(e);
        free(call);
        return NULL;
      }
      snprintf(call->u.call.callee, sizeof(call->u.call.callee), "%s", e->u.var_name);
      free_expr(e);
    if (!lex_next(C)) return NULL;
      args = (OlExpr **)malloc(cap * sizeof(OlExpr *));
      if (!args) {
        errf(C, "oom");
        free(call);
        return NULL;
      }
      if (C->L->tok != TOK_RPAREN) {
        for (;;) {
          OlExpr *a = parse_expr(C);
          if (!a) {
            size_t i;
            for (i = 0; i < n; ++i) free_expr(args[i]);
            free(args);
            free(call);
            return NULL;
          }
          if (n >= cap) {
            cap *= 2;
            args = (OlExpr **)realloc(args, cap * sizeof(OlExpr *));
            if (!args) {
              errf(C, "oom");
              free_expr(a);
              free(call);
              return NULL;
            }
          }
          args[n++] = a;
          if (C->L->tok == TOK_COMMA) {
    if (!lex_next(C)) return NULL;
            continue;
          }
          break;
        }
      }
      if (!expect(C, TOK_RPAREN)) {
        size_t i;
        for (i = 0; i < n; ++i) free_expr(args[i]);
        free(args);
        free(call);
        return NULL;
      }
      call->u.call.args = args;
      call->u.call.arg_count = n;
      e = call;
      continue;
    }
    if (C->L->tok == TOK_DOT) {
      OlExpr *fld = new_expr(OL_EX_FIELD, e->line);
      if (!fld) {
        free_expr(e);
        return NULL;
      }
      fld->u.field.obj = e;
      fld->u.field.base_ty[0] = '\0';
    if (!lex_next(C)) return NULL;
      if (C->L->tok != TOK_IDENT) {
        errf(C, "expected field name");
        free_expr(fld);
        return NULL;
      }
      snprintf(fld->u.field.field, sizeof(fld->u.field.field), "%s", C->L->ident);
    if (!lex_next(C)) return NULL;
      e = fld;
      continue;
    }
    if (C->L->tok == TOK_LBRACK) {
      OlExpr *ix = new_expr(OL_EX_INDEX, e->line);
      if (!ix) {
        free_expr(e);
        return NULL;
      }
      ix->u.index_.arr = e;
      ix->u.index_.arr_ty[0] = '\0';
    if (!lex_next(C)) return NULL;
      ix->u.index_.index_expr = parse_expr(C);
      if (!ix->u.index_.index_expr) {
        free_expr(ix);
        return NULL;
      }
      if (!expect(C, TOK_RBRACK)) {
        free_expr(ix);
        return NULL;
      }
      e = ix;
      continue;
    }
    break;
  }
  return e;
}

static OlExpr *parse_postfix(ParseCtx *C) {
  OlExpr *e = parse_primary(C);
  if (!e) return NULL;
  return parse_postfix_chain(C, e);
}

static OlExpr *parse_mul_rest(ParseCtx *C, OlExpr *l) {
  while (C->L->tok == TOK_STAR || C->L->tok == TOK_SLASH || C->L->tok == TOK_PERCENT) {
    OlBinOp op = OL_BIN_MUL;
    int line = C->L->line;
    if (C->L->tok == TOK_SLASH) op = OL_BIN_DIV;
    if (C->L->tok == TOK_PERCENT) op = OL_BIN_MOD;
    if (!lex_next(C)) return NULL;
    {
      OlExpr *r = parse_unary(C);
      OlExpr *p;
      if (!r) {
        free_expr(l);
        return NULL;
      }
      p = new_expr(OL_EX_BINARY, line);
      if (!p) {
        free_expr(l);
        free_expr(r);
        return NULL;
      }
      p->u.binary.op = op;
      p->u.binary.left = l;
      p->u.binary.right = r;
      l = p;
    }
  }
  return l;
}

static OlExpr *parse_add_rest(ParseCtx *C, OlExpr *l) {
  while (C->L->tok == TOK_PLUS || C->L->tok == TOK_MINUS) {
    OlBinOp op = C->L->tok == TOK_PLUS ? OL_BIN_ADD : OL_BIN_SUB;
    int line = C->L->line;
    if (!lex_next(C)) return NULL;
    {
      OlExpr *r = parse_mul(C);
      OlExpr *p;
      if (!r) {
        free_expr(l);
        return NULL;
      }
      p = new_expr(OL_EX_BINARY, line);
      if (!p) {
        free_expr(l);
        free_expr(r);
        return NULL;
      }
      p->u.binary.op = op;
      p->u.binary.left = l;
      p->u.binary.right = r;
      l = p;
    }
  }
  return l;
}

static OlExpr *parse_cmp_rest(ParseCtx *C, OlExpr *l) {
  while (C->L->tok == TOK_EQEQ || C->L->tok == TOK_NE || C->L->tok == TOK_LT || C->L->tok == TOK_GT || C->L->tok == TOK_LE ||
         C->L->tok == TOK_GE) {
    OlBinOp op = OL_BIN_EQ;
    int line = C->L->line;
    if (C->L->tok == TOK_NE) op = OL_BIN_NE;
    else if (C->L->tok == TOK_LT)
      op = OL_BIN_LT;
    else if (C->L->tok == TOK_GT)
      op = OL_BIN_GT;
    else if (C->L->tok == TOK_LE)
      op = OL_BIN_LE;
    else if (C->L->tok == TOK_GE)
      op = OL_BIN_GE;
    if (!lex_next(C)) return NULL;
    {
      OlExpr *r = parse_add(C);
      OlExpr *p;
      if (!r) {
        free_expr(l);
        return NULL;
      }
      p = new_expr(OL_EX_BINARY, line);
      if (!p) {
        free_expr(l);
        free_expr(r);
        return NULL;
      }
      p->u.binary.op = op;
      p->u.binary.left = l;
      p->u.binary.right = r;
      l = p;
    }
  }
  return l;
}

static OlExpr *parse_cmp(ParseCtx *C);
static OlExpr *parse_and(ParseCtx *C);
static OlExpr *parse_or(ParseCtx *C);

static OlExpr *parse_and_rest(ParseCtx *C, OlExpr *l) {
  while (C->L->tok == TOK_AMPAMP) {
    int line = C->L->line;
    OlExpr *r, *p;
    if (!lex_next(C)) return NULL;
    r = parse_cmp(C);
    if (!r) { free_expr(l); return NULL; }
    p = new_expr(OL_EX_BINARY, line);
    if (!p) { free_expr(l); free_expr(r); return NULL; }
    p->u.binary.op = OL_BIN_AND;
    p->u.binary.left = l;
    p->u.binary.right = r;
    l = p;
  }
  return l;
}

static OlExpr *parse_or_rest(ParseCtx *C, OlExpr *l) {
  while (C->L->tok == TOK_PIPEPIPE) {
    int line = C->L->line;
    OlExpr *r, *p;
    if (!lex_next(C)) return NULL;
    r = parse_cmp(C);
    if (!r) { free_expr(l); return NULL; }
    r = parse_and_rest(C, r);
    if (!r) { free_expr(l); return NULL; }
    p = new_expr(OL_EX_BINARY, line);
    if (!p) { free_expr(l); free_expr(r); return NULL; }
    p->u.binary.op = OL_BIN_OR;
    p->u.binary.left = l;
    p->u.binary.right = r;
    l = p;
  }
  return l;
}

static OlExpr *parse_expr_from_atom(ParseCtx *C, OlExpr *atom) {
  OlExpr *e;
  if (!atom) return NULL;
  e = parse_postfix_chain(C, atom);
  if (!e) return NULL;
  e = parse_mul_rest(C, e);
  if (!e) return NULL;
  e = parse_add_rest(C, e);
  if (!e) return NULL;
  e = parse_cmp_rest(C, e);
  if (!e) return NULL;
  e = parse_and_rest(C, e);
  if (!e) return NULL;
  return parse_or_rest(C, e);
}

static OlExpr *parse_unary(ParseCtx *C) {
  if (C->L->tok == TOK_MINUS) {
    int line = C->L->line;
    OlExpr *z, *b;
    if (!lex_next(C)) return NULL;
    b = parse_unary(C);
    if (!b) return NULL;
    z = new_expr(OL_EX_NEG, line);
    if (!z) {
      free_expr(b);
      return NULL;
    }
    z->u.neg.inner = b;
    return z;
  }
  if (C->L->tok == TOK_PLUS) {
    if (!lex_next(C)) return NULL;
    return parse_unary(C);
  }
  if (C->L->tok == TOK_BANG) {
    int line = C->L->line;
    OlExpr *inner, *e;
    if (!lex_next(C)) return NULL;
    inner = parse_unary(C);
    if (!inner) return NULL;
    e = new_expr(OL_EX_NOT, line);
    if (!e) { free_expr(inner); return NULL; }
    e->u.not_.inner = inner;
    return e;
  }
  return parse_postfix(C);
}

static OlExpr *parse_mul(ParseCtx *C) {
  OlExpr *l = parse_unary(C);
  if (!l) return NULL;
  return parse_mul_rest(C, l);
}

static OlExpr *parse_add(ParseCtx *C) {
  OlExpr *l = parse_mul(C);
  if (!l) return NULL;
  return parse_add_rest(C, l);
}

static OlExpr *parse_cmp(ParseCtx *C) {
  OlExpr *l = parse_add(C);
  if (!l) return NULL;
  return parse_cmp_rest(C, l);
}

static OlExpr *parse_and(ParseCtx *C) {
  OlExpr *l = parse_cmp(C);
  if (!l) return NULL;
  return parse_and_rest(C, l);
}

static OlExpr *parse_or(ParseCtx *C) {
  OlExpr *l = parse_and(C);
  if (!l) return NULL;
  return parse_or_rest(C, l);
}

static OlExpr *parse_expr(ParseCtx *C) { return parse_or(C); }

static OlStmt *parse_stmt(ParseCtx *C);

static OlStmt *parse_block(ParseCtx *C) {
  OlStmt *blk = new_stmt(OL_ST_BLOCK, C->L->line);
  OlStmt **tail;
  if (!blk) return NULL;
  tail = &blk->u.block.first;
  if (!expect(C, TOK_LBRACE)) {
    free(blk);
    return NULL;
  }
  while (C->L->tok != TOK_RBRACE && C->L->tok != TOK_EOF) {
    OlStmt *s = parse_stmt(C);
    if (!s) {
      free_stmt(blk->u.block.first);
      free(blk);
      return NULL;
    }
    *tail = s;
    while (*tail && (*tail)->next) tail = &(*tail)->next;
    tail = &(*tail)->next;
  }
  if (!expect(C, TOK_RBRACE)) {
    free_stmt(blk->u.block.first);
    free(blk);
    return NULL;
  }
  return blk;
}

static OlStmt *parse_stmt(ParseCtx *C) {
  int line = C->L->line;
  if (C->L->tok == TOK_LBRACE) return parse_block(C);
  if (C->L->tok == TOK_IDENT) {
    OlExpr *atom;
    char iname[128];
    snprintf(iname, sizeof(iname), "%s", C->L->ident);
    atom = new_expr(OL_EX_VAR, line);
    if (!atom) return NULL;
    snprintf(atom->u.var_name, sizeof(atom->u.var_name), "%s", iname);
    if (!lex_next(C)) return NULL;
    {
      OlStmt *s;
      OlExpr *e = parse_expr_from_atom(C, atom);
      if (!e) return NULL;
      /* Check for assignment: expr = expr ; */
      if (C->L->tok == TOK_EQ) {
        OlStmt *as = new_stmt(OL_ST_ASSIGN, line);
        if (!as) {
          free_expr(e);
          return NULL;
        }
        as->u.assign_.lhs = e;  /* e is the left value */
        if (!lex_next(C)) { free_expr(e); free(as); return NULL; }
        as->u.assign_.rhs = parse_expr(C);
        if (!as->u.assign_.rhs) {
          free_expr(e);
          free(as);
          return NULL;
        }
        if (!expect(C, TOK_SEMI)) {
          free_expr(as->u.assign_.lhs);
          free_expr(as->u.assign_.rhs);
          free(as);
          return NULL;
        }
        return as;
      }
      /* Not assignment, treat as expression statement */
      s = new_stmt(OL_ST_EXPR, line);
      if (!s) {
        free_expr(e);
        return NULL;
      }
      s->u.expr = e;
      if (!expect(C, TOK_SEMI)) {
        free_expr(e);
        free(s);
        return NULL;
      }
      return s;
    }
  }
  if (C->L->tok == TOK_KW_LET) {
    OlStmt *s = new_stmt(OL_ST_LET, line);
    if (!s) return NULL;
    if (!lex_next(C)) return NULL;
    if (C->L->tok != TOK_IDENT) {
      errf(C, "expected name in let");
      free(s);
      return NULL;
    }
    snprintf(s->u.let_.name, sizeof(s->u.let_.name), "%s", C->L->ident);
    if (!lex_next(C)) return NULL;
    if (!expect(C, TOK_COLON)) {
      free(s);
      return NULL;
    }
    if (!parse_type_ref(C, &s->u.let_.ty)) {
      free(s);
      return NULL;
    }
    /* Optional initializer: scalar types require init, aggregate types don't */
    if (C->L->tok == TOK_EQ) {
      if (!lex_next(C)) return NULL;
      s->u.let_.init = parse_expr(C);
      if (!s->u.let_.init) {
        free(s);
        return NULL;
      }
    } else {
      s->u.let_.init = NULL;
    }
    if (!expect(C, TOK_SEMI)) {
      free_expr(s->u.let_.init);
      free(s);
      return NULL;
    }
    return s;
  }
  if (C->L->tok == TOK_KW_BREAK) {
    OlStmt *s = new_stmt(OL_ST_BREAK, line);
    if (!s) return NULL;
    if (!lex_next(C)) return NULL;
    if (!expect(C, TOK_SEMI)) { free(s); return NULL; }
    return s;
  }
  if (C->L->tok == TOK_KW_CONTINUE) {
    OlStmt *s = new_stmt(OL_ST_CONTINUE, line);
    if (!s) return NULL;
    if (!lex_next(C)) return NULL;
    if (!expect(C, TOK_SEMI)) { free(s); return NULL; }
    return s;
  }
  if (C->L->tok == TOK_KW_IF) {
    OlStmt *s = new_stmt(OL_ST_IF, line);
    if (!s) return NULL;
    if (!lex_next(C)) return NULL;
    if (!expect(C, TOK_LPAREN)) {
      free(s);
      return NULL;
    }
    s->u.if_.cond = parse_expr(C);
    if (!s->u.if_.cond) {
      free(s);
      return NULL;
    }
    if (!expect(C, TOK_RPAREN)) {
      free_expr(s->u.if_.cond);
      free(s);
      return NULL;
    }
    s->u.if_.then_arm = parse_stmt(C);
    if (!s->u.if_.then_arm) {
      free_expr(s->u.if_.cond);
      free(s);
      return NULL;
    }
    if (C->L->tok == TOK_KW_ELSE) {
      if (!lex_next(C)) return NULL;
      s->u.if_.else_arm = parse_stmt(C);
      if (!s->u.if_.else_arm) {
        free_stmt(s->u.if_.then_arm);
        free_expr(s->u.if_.cond);
        free(s);
        return NULL;
      }
    }
    return s;
  }
  if (C->L->tok == TOK_KW_WHILE) {
    OlStmt *s = new_stmt(OL_ST_WHILE, line);
    if (!s) return NULL;
    if (!lex_next(C)) return NULL;
    if (!expect(C, TOK_LPAREN)) {
      free(s);
      return NULL;
    }
    s->u.while_.cond = parse_expr(C);
    if (!s->u.while_.cond) {
      free(s);
      return NULL;
    }
    if (!expect(C, TOK_RPAREN)) {
      free_expr(s->u.while_.cond);
      free(s);
      return NULL;
    }
    s->u.while_.body = parse_stmt(C);
    if (!s->u.while_.body) {
      free_expr(s->u.while_.cond);
      free(s);
      return NULL;
    }
    return s;
  }
  if (C->L->tok == TOK_KW_RETURN) {
    OlStmt *s = new_stmt(OL_ST_RETURN, line);
    if (!s) return NULL;
    if (!lex_next(C)) return NULL;
    if (C->L->tok == TOK_SEMI) {
      s->u.ret.val = NULL;
      if (!lex_next(C)) return NULL;
      return s;
    }
    s->u.ret.val = parse_expr(C);
    if (!s->u.ret.val) {
      free(s);
      return NULL;
    }
    if (!expect(C, TOK_SEMI)) {
      free_expr(s->u.ret.val);
      free(s);
      return NULL;
    }
    return s;
  }
  if (C->L->tok == TOK_KW_PTRBIND) {
    OlStmt *s = new_stmt(OL_ST_PTRBIND, line);
    if (!s) return NULL;
    if (!lex_next(C)) return NULL;
    if (C->L->tok != TOK_IDENT) {
      errf(C, "ptrbind name");
      free(s);
      return NULL;
    }
    snprintf(s->u.ptrbind.bind, sizeof(s->u.ptrbind.bind), "%s", C->L->ident);
    if (!lex_next(C)) return NULL;
    if (!expect(C, TOK_KW_AS)) {
      free(s);
      return NULL;
    }
    if (!parse_type_ref(C, &s->u.ptrbind.ty)) {
      free(s);
      return NULL;
    }
    if (!expect(C, TOK_KW_FROM)) {
      free(s);
      return NULL;
    }
    if (C->L->tok != TOK_IDENT) {
      errf(C, "ptrbind symbol");
      free(s);
      return NULL;
    }
    snprintf(s->u.ptrbind.symbol, sizeof(s->u.ptrbind.symbol), "%s", C->L->ident);
    if (!lex_next(C)) return NULL;
    if (!expect(C, TOK_SEMI)) {
      free(s);
      return NULL;
    }
    return s;
  }
  if (C->L->tok == TOK_KW_DEREF) {
    OlStmt *s = new_stmt(OL_ST_DEREF, line);
    if (!s) return NULL;
    if (!lex_next(C)) return NULL;
    if (C->L->tok != TOK_IDENT) {
      errf(C, "deref name");
      free(s);
      return NULL;
    }
    snprintf(s->u.deref.bind, sizeof(s->u.deref.bind), "%s", C->L->ident);
    if (!lex_next(C)) return NULL;
    if (!expect(C, TOK_KW_AS)) {
      free(s);
      return NULL;
    }
    if (!parse_type_ref(C, &s->u.deref.ty)) {
      free(s);
      return NULL;
    }
    if (!expect(C, TOK_SEMI)) {
      free(s);
      return NULL;
    }
    return s;
  }
  {
    OlStmt *s = new_stmt(OL_ST_EXPR, line);
    OlExpr *e;
    if (!s) return NULL;
    e = parse_expr(C);
    if (!e) {
      free(s);
      return NULL;
    }
    s->u.expr = e;
    if (!expect(C, TOK_SEMI)) {
      free_expr(e);
      free(s);
      return NULL;
    }
    return s;
  }
}

static int is_builtin_type_name(const char *t) {
  return strcmp(t, "bool") == 0 || strcmp(t, "u8") == 0 || strcmp(t, "i32") == 0 || strcmp(t, "u32") == 0 ||
         strcmp(t, "i64") == 0 || strcmp(t, "u64") == 0 || strcmp(t, "ptr") == 0;
}

static int parse_typedef(ParseCtx *C) {
  char name[128];
  if (!expect(C, TOK_KW_TYPE)) return 0;
  if (C->L->tok != TOK_IDENT) {
    errf(C, "typedef name");
    return 0;
  }
  snprintf(name, sizeof(name), "%s", C->L->ident);
    if (!lex_next(C)) return 0;
  if (!expect(C, TOK_EQ)) return 0;
  if (C->L->tok == TOK_KW_STRUCT) {
    char rest[2048];
    size_t rn = 0;
    int idx;
    if (!lex_next(C)) return 0;
    if (!expect(C, TOK_LBRACE)) return 0;
    rest[0] = '\0';
    while (C->L->tok != TOK_RBRACE && C->L->tok != TOK_EOF) {
      char fname[128], ftype[128];
      if (C->L->tok != TOK_IDENT) {
        errf(C, "field name");
        return 0;
      }
      snprintf(fname, sizeof(fname), "%s", C->L->ident);
    if (!lex_next(C)) return 0;
      if (!expect(C, TOK_COLON)) return 0;
      if (C->L->tok != TOK_IDENT && !is_builtin_type_tok(C->L->tok)) {
        errf(C, "field type");
        return 0;
      }
      if (is_builtin_type_tok(C->L->tok)) {
        if (C->L->tok == TOK_KW_BOOL) snprintf(ftype, sizeof(ftype), "bool");
        else if (C->L->tok == TOK_KW_U8)
          snprintf(ftype, sizeof(ftype), "u8");
        else if (C->L->tok == TOK_KW_I32)
          snprintf(ftype, sizeof(ftype), "i32");
        else if (C->L->tok == TOK_KW_U32)
          snprintf(ftype, sizeof(ftype), "u32");
        else if (C->L->tok == TOK_KW_I64)
          snprintf(ftype, sizeof(ftype), "i64");
        else if (C->L->tok == TOK_KW_U64)
          snprintf(ftype, sizeof(ftype), "u64");
        else if (C->L->tok == TOK_KW_PTR)
          snprintf(ftype, sizeof(ftype), "ptr");
        else {
          errf(C, "bad field type");
          return 0;
        }
    if (!lex_next(C)) return 0;
      } else {
        snprintf(ftype, sizeof(ftype), "%s", C->L->ident);
    if (!lex_next(C)) return 0;
      }
      {
        size_t need = strlen(fname) + strlen(ftype) + 4;
        if (rn + need >= sizeof(rest)) {
          errf(C, "struct too large");
          return 0;
        }
        if (rn > 0) {
          rest[rn++] = ',';
          rest[rn] = '\0';
        }
        snprintf(rest + rn, sizeof(rest) - rn, "%s:%s", fname, ftype);
        rn = strlen(rest);
      }
      if (C->L->tok == TOK_COMMA) {
        if (!lex_next(C)) return 0;
      }
    }
    if (!expect(C, TOK_RBRACE)) return 0;
    idx = ol_program_add_typedef_struct(C->prog, name);
    if (idx < 0) {
      errf(C, "duplicate struct");
      return 0;
    }
    {
      char *tokp, *save = NULL;
      char buf[2048];
      snprintf(buf, sizeof(buf), "%s", rest);
      for (tokp = strtok_r(buf, ",", &save); tokp; tokp = strtok_r(NULL, ",", &save)) {
        char *colon = strchr(tokp, ':');
        char ft[128];
        if (!colon) {
          errf(C, "bad field");
          return 0;
        }
        *colon = '\0';
        while (*tokp == ' ') tokp++;
        snprintf(ft, sizeof(ft), "%s", colon + 1);
        if (!is_builtin_type_name(ft) && ol_program_find_typedef(C->prog, ft) < 0) {
          errf(C, "unknown field type");
          return 0;
        }
        if (!ol_program_add_typedef_struct_field(C->prog, (size_t)idx, tokp, ft)) {
          errf(C, "struct field fail");
          return 0;
        }
      }
    }
    if (C->L->tok == TOK_SEMI) {
      if (!lex_next(C)) return 0;
    }
    return 1;
  }
  if (C->L->tok == TOK_KW_ARRAY) {
    char elem[128];
    unsigned count = 0;
    if (!lex_next(C)) return 0;
    if (!expect(C, TOK_LT)) return 0;
    if (C->L->tok == TOK_IDENT)
      snprintf(elem, sizeof(elem), "%s", C->L->ident);
    else if (C->L->tok == TOK_KW_BOOL) {
      snprintf(elem, sizeof(elem), "bool");
    } else if (C->L->tok == TOK_KW_U8) {
      snprintf(elem, sizeof(elem), "u8");
    } else if (C->L->tok == TOK_KW_U32) {
      snprintf(elem, sizeof(elem), "u32");
    } else if (C->L->tok == TOK_KW_U64) {
      snprintf(elem, sizeof(elem), "u64");
    }
    else if (C->L->tok == TOK_KW_I32) {
      snprintf(elem, sizeof(elem), "i32");
    } else if (C->L->tok == TOK_KW_I64) {
      snprintf(elem, sizeof(elem), "i64");
    } else if (C->L->tok == TOK_KW_PTR) {
      snprintf(elem, sizeof(elem), "ptr");
    } else {
      errf(C, "array elem");
      return 0;
    }
    if (!lex_next(C)) return 0;
    if (!expect(C, TOK_COMMA)) return 0;
    if (C->L->tok != TOK_INT) {
      errf(C, "array count");
      return 0;
    }
    count = (unsigned)C->L->int_val;
    if (!lex_next(C)) return 0;
    if (!expect(C, TOK_GT)) return 0;
    if (!is_builtin_type_name(elem) && ol_program_find_typedef(C->prog, elem) < 0) {
      errf(C, "unknown elem");
      return 0;
    }
    if (!ol_program_add_typedef_array(C->prog, name, elem, count)) {
      errf(C, "duplicate array");
      return 0;
    }
    if (C->L->tok == TOK_SEMI) {
      if (!lex_next(C)) return 0;
    }
    return 1;
  }
  errf(C, "bad typedef");
  return 0;
}

static int parse_fn_decl_header(ParseCtx *C, char *name_out, OlTypeRef *ret_out, OlParam **params_out, size_t *pc_out) {
  OlParam *params = NULL;
  size_t pc = 0, cap = 0;
  if (C->L->tok != TOK_IDENT) {
    errf(C, "function name expected");
    return 0;
  }
  snprintf(name_out, 128, "%s", C->L->ident);
    if (!lex_next(C)) return 0;
  if (!expect(C, TOK_LPAREN)) return 0;
  if (C->L->tok != TOK_RPAREN) {
    for (;;) {
      OlParam *np;
      if (C->L->tok != TOK_IDENT) {
        errf(C, "param name");
        free(params);
        return 0;
      }
      if (pc >= cap) {
        size_t ncap = cap ? cap * 2 : 4;
        OlParam *p2 = (OlParam *)realloc(params, ncap * sizeof(OlParam));
        if (!p2) {
          free(params);
          errf(C, "oom");
          return 0;
        }
        params = p2;
        cap = ncap;
      }
      np = &params[pc];
      memset(np, 0, sizeof(*np));
      snprintf(np->name, sizeof(np->name), "%s", C->L->ident);
    if (!lex_next(C)) return 0;
      if (!expect(C, TOK_COLON)) {
        free(params);
        return 0;
      }
      if (!parse_type_ref(C, &np->ty)) {
        free(params);
        return 0;
      }
      pc++;
      if (C->L->tok == TOK_COMMA) {
    if (!lex_next(C)) return 0;
        continue;
      }
      break;
    }
  }
  if (!expect(C, TOK_RPAREN)) {
    free(params);
    return 0;
  }
  ret_out->kind = OL_TY_VOID;
  if (C->L->tok == TOK_MINUS) {
    if (!lex_next(C)) return 0;
    if (!expect(C, TOK_GT)) {
      free(params);
      return 0;
    }
    if (!parse_type_ref(C, ret_out)) {
      free(params);
      return 0;
    }
  }
  *params_out = params;
  *pc_out = pc;
  return 1;
}

static int parse_global_let(ParseCtx *C, OlGlobalSection sec, const char *custom_sec) {
  OlGlobalDef g;
  OlGlobalDef *ng;
  memset(&g, 0, sizeof(g));
  g.section = sec;
  if (custom_sec && custom_sec[0]) snprintf(g.custom_section, sizeof(g.custom_section), "%s", custom_sec);
  if (!expect(C, TOK_KW_LET)) return 0;
  if (C->L->tok != TOK_IDENT) {
    errf(C, "global name");
    return 0;
  }
  snprintf(g.name, sizeof(g.name), "%s", C->L->ident);
    if (!lex_next(C)) return 0;
  if (!expect(C, TOK_COLON)) return 0;
  if (!parse_type_ref(C, &g.ty)) return 0;
  g.init = NULL;
  if (C->L->tok == TOK_EQ) {
    if (!lex_next(C)) return 0;
    g.init = parse_expr(C);
    if (!g.init) return 0;
  }
  if (!expect(C, TOK_SEMI)) {
    free_expr(g.init);
    return 0;
  }
  ng = (OlGlobalDef *)realloc(C->prog->globals, (C->prog->global_count + 1) * sizeof(OlGlobalDef));
  if (!ng) {
    free_expr(g.init);
    errf(C, "oom");
    return 0;
  }
  C->prog->globals = ng;
  C->prog->globals[C->prog->global_count] = g;
  C->prog->global_count++;
  return 1;
}

static int parse_top_global(ParseCtx *C) {
  OlGlobalSection sec = OL_GSEC_DEFAULT;
  char custom[128] = "";
  while (C->L->tok == TOK_AT) {
    if (!lex_next(C)) return 0;
    if (C->L->tok != TOK_IDENT) {
      errf(C, "expected attribute name after @");
      return 0;
    }
    if (strcmp(C->L->ident, "data") == 0) {
      sec = OL_GSEC_DATA;
    if (!lex_next(C)) return 0;
    } else if (strcmp(C->L->ident, "bss") == 0) {
      sec = OL_GSEC_BSS;
    if (!lex_next(C)) return 0;
    } else if (strcmp(C->L->ident, "rodata") == 0) {
      sec = OL_GSEC_RODATA;
    if (!lex_next(C)) return 0;
    } else if (strcmp(C->L->ident, "section") == 0) {
    if (!lex_next(C)) return 0;
      if (!expect(C, TOK_LPAREN)) return 0;
      if (C->L->tok != TOK_STR) {
        errf(C, "@section(\"name\") expected");
        return 0;
      }
      snprintf(custom, sizeof(custom), "%s", C->L->str_val);
    if (!lex_next(C)) return 0;
      if (!expect(C, TOK_RPAREN)) return 0;
      sec = OL_GSEC_CUSTOM;
    } else {
      errf(C, "unknown @ attribute");
      return 0;
    }
  }
  return parse_global_let(C, sec, custom[0] ? custom : NULL);
}

static int parse_extern_item(ParseCtx *C) {
  OlExternDecl ex;
  OlExternDecl *ne;
  OlFuncDef fn;
  OlFuncDef *nf;
  OlParam *params = NULL;
  size_t pc = 0;
  OlTypeRef ret;
  char name[128];
  if (!expect(C, TOK_KW_EXTERN)) return 0;
  if (!expect(C, TOK_KW_FN)) return 0;
  if (!parse_fn_decl_header(C, name, &ret, &params, &pc)) return 0;
  if (C->L->tok == TOK_SEMI) {
    memset(&ex, 0, sizeof(ex));
    snprintf(ex.name, sizeof(ex.name), "%s", name);
    ex.ret = ret;
    ex.params = params;
    ex.param_count = pc;
    if (!lex_next(C)) return 0;
    ne = (OlExternDecl *)realloc(C->prog->externs, (C->prog->extern_count + 1) * sizeof(OlExternDecl));
    if (!ne) {
      free(params);
      errf(C, "oom");
      return 0;
    }
    C->prog->externs = ne;
    C->prog->externs[C->prog->extern_count] = ex;
    C->prog->extern_count++;
    return 1;
  }
  if (C->L->tok == TOK_LBRACE) {
    memset(&fn, 0, sizeof(fn));
    snprintf(fn.name, sizeof(fn.name), "%s", name);
    fn.ret = ret;
    fn.params = params;
    fn.param_count = pc;
    fn.is_export = 1;
    fn.body = parse_block(C);
    if (!fn.body) {
      free(params);
      return 0;
    }
    nf = (OlFuncDef *)realloc(C->prog->funcs, (C->prog->func_count + 1) * sizeof(OlFuncDef));
    if (!nf) {
      free(params);
      free_stmt(fn.body);
      errf(C, "oom");
      return 0;
    }
    C->prog->funcs = nf;
    C->prog->funcs[C->prog->func_count] = fn;
    C->prog->func_count++;
    return 1;
  }
  errf(C, "expected ; or { after extern fn header");
  free(params);
  return 0;
}

static int parse_fn(ParseCtx *C) {
  OlFuncDef fn;
  OlFuncDef *nf;
  memset(&fn, 0, sizeof(fn));
  if (!expect(C, TOK_KW_FN)) return 0;
  if (!parse_fn_decl_header(C, fn.name, &fn.ret, &fn.params, &fn.param_count)) return 0;
  fn.is_export = 0;
  fn.body = parse_block(C);
  if (!fn.body) {
    free(fn.params);
    return 0;
  }
  nf = (OlFuncDef *)realloc(C->prog->funcs, (C->prog->func_count + 1) * sizeof(OlFuncDef));
  if (!nf) {
    free(fn.params);
    free_stmt(fn.body);
    errf(C, "oom");
    return 0;
  }
  C->prog->funcs = nf;
  C->prog->funcs[C->prog->func_count] = fn;
  C->prog->func_count++;
  return 1;
}

int ol_parse_source_file(const char *path, OlProgram *out, char *err, size_t err_len) {
  FILE *f;
  char *buf = NULL;
  size_t n = 0;
  long sz;
  OlLexer Lex;
  ParseCtx C;

  ol_program_init(out);
  f = fopen(path, "rb");
  if (!f) {
    snprintf(err, err_len, "cannot open source: %s", path);
    return 0;
  }
  fseek(f, 0, SEEK_END);
  sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (sz < 0) {
    fclose(f);
    snprintf(err, err_len, "cannot read: %s", path);
    return 0;
  }
  buf = (char *)malloc((size_t)sz + 1);
  if (!buf) {
    fclose(f);
    snprintf(err, err_len, "oom");
    return 0;
  }
  n = fread(buf, 1, (size_t)sz, f);
  fclose(f);
  buf[n] = '\0';

  if (strlen(path) >= sizeof(out->source_path)) {
    free(buf);
    snprintf(err, err_len, "source path too long (max %zu bytes)", sizeof(out->source_path) - 1);
    ol_program_free(out);
    return 0;
  }

  ol_lexer_init(&Lex, path, buf, n);
  ol_lexer_next(&Lex);
  if (Lex.lexer_error) {
    snprintf(err, err_len, "%s:%d: %s", Lex.path ? Lex.path : "", Lex.line, Lex.errmsg);
    free(buf);
    ol_program_free(out);
    return 0;
  }
  C.prog = out;
  snprintf(out->source_path, sizeof(out->source_path), "%s", path);
  C.L = &Lex;
  C.err = err;
  C.err_len = err_len;

  while (C.L->tok != TOK_EOF) {
    if (C.L->tok == TOK_KW_EXTERN) {
      if (!parse_extern_item(&C)) goto fail;
      continue;
    }
    if (C.L->tok == TOK_AT) {
      if (!parse_top_global(&C)) goto fail;
      continue;
    }
    if (C.L->tok == TOK_KW_LET) {
      if (!parse_global_let(&C, OL_GSEC_DEFAULT, NULL)) goto fail;
      continue;
    }
    if (C.L->tok == TOK_KW_TYPE) {
      if (!parse_typedef(&C)) goto fail;
      continue;
    }
    if (C.L->tok == TOK_KW_FN) {
      if (!parse_fn(&C)) goto fail;
      continue;
    }
    errf(&C, "top-level expected extern fn, @attrs let, let, type, or fn");
    goto fail;
  }

  free(buf);
  ol_lexer_free_strings(&Lex);
  if (out->func_count == 0) {
    snprintf(err, err_len, "no function defined");
    ol_program_free(out);
    return 0;
  }
  {
    size_t i;
    int found = 0;
    for (i = 0; i < out->func_count; ++i) {
      if (strcmp(out->funcs[i].name, "main") == 0) {
        snprintf(out->entry_name, sizeof(out->entry_name), "%s", "main");
        found = 1;
        break;
      }
    }
    if (!found) snprintf(out->entry_name, sizeof(out->entry_name), "%s", out->funcs[out->func_count - 1].name);
  }
  {
    size_t ei;
    for (ei = 0; ei < out->func_count; ++ei) {
      if (strcmp(out->funcs[ei].name, out->entry_name) != 0) continue;
      if (!out->funcs[ei].is_export) {
        snprintf(err, err_len, "entry function %s must be exported (use extern fn ... { })", out->entry_name);
        ol_program_free(out);
        return 0;
      }
      break;
    }
  }
  return 1;

fail:
  free(buf);
  ol_lexer_free_strings(&Lex);
  ol_program_free(out);
  return 0;
}

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
    case OL_EX_LOAD:
      free_expr(e->u.load.ptr);
      break;
    case OL_EX_STORE:
      free_expr(e->u.store.ptr);
      free_expr(e->u.store.val);
      break;
    case OL_EX_FIELD:
      free_expr(e->u.field.obj);
      break;
    case OL_EX_INDEX:
      free_expr(e->u.index_.arr);
      free_expr(e->u.index_.index_expr);
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
      case OL_ST_BLOCK:
        free_stmt(s->u.block.first);
        break;
      case OL_ST_LET:
        free_expr(s->u.let_.init);
        break;
      case OL_ST_ASSIGN:
        free_expr(s->u.assign_.lhs);
        free_expr(s->u.assign_.rhs);
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
