#ifndef OLANG_FRONTEND_AST_H
#define OLANG_FRONTEND_AST_H

#include <stddef.h>
#include <stdint.h>

typedef struct OlProgram OlProgram;

typedef enum OlTyKind {
  OL_TY_VOID = 0,
  OL_TY_BOOL,
  OL_TY_U8,
  OL_TY_I8,
  OL_TY_U16,
  OL_TY_I16,
  OL_TY_I32,
  OL_TY_U32,
  OL_TY_I64,
  OL_TY_U64,
  OL_TY_F16,
  OL_TY_F32,
  OL_TY_F64,
  OL_TY_B8,
  OL_TY_B16,
  OL_TY_B32,
  OL_TY_B64,
  OL_TY_PTR,
  OL_TY_ALIAS
} OlTyKind;

typedef struct OlTypeRef {
  OlTyKind kind;
  char alias_name[128]; /* if OL_TY_ALIAS */
} OlTypeRef;

typedef struct OlParam {
  char name[128];
  OlTypeRef ty;
} OlParam;

typedef struct OlExternDecl {
  char name[128];
  OlTypeRef ret;
  OlParam *params;
  size_t param_count;
} OlExternDecl;

typedef enum OlExprKind {
  OL_EX_INT = 1,
  OL_EX_FLOAT,
  OL_EX_STR,
  OL_EX_BOOL,
  OL_EX_CHAR,
  OL_EX_VAR,
  OL_EX_NEG,
  OL_EX_NOT,
  OL_EX_BNOT,
  OL_EX_BINARY,
  OL_EX_CALL,
  OL_EX_CAST,
  OL_EX_REINTERPRET,
  OL_EX_ADDR,
  OL_EX_LOAD,
  OL_EX_STORE,
  OL_EX_FIELD,
  OL_EX_INDEX
} OlExprKind;

typedef enum OlBinOp {
  OL_BIN_ADD,
  OL_BIN_SUB,
  OL_BIN_MUL,
  OL_BIN_DIV,
  OL_BIN_MOD,
  OL_BIN_EQ,
  OL_BIN_NE,
  OL_BIN_LT,
  OL_BIN_GT,
  OL_BIN_LE,
  OL_BIN_GE,
  OL_BIN_AND,
  OL_BIN_OR,
  OL_BIN_BAND,
  OL_BIN_BOR,
  OL_BIN_BXOR,
  OL_BIN_SHL,
  OL_BIN_SHR
} OlBinOp;

typedef struct OlExpr OlExpr;
struct OlExpr {
  OlExprKind kind;
  OlTypeRef ty; /* filled by sema */
  int line;
  union {
    struct {
      int64_t int_val;
      uint8_t int_suffix; /* 0=none, 1=i32, 2=i64, 3=u8, 4=u32, 5=u64, 6=i8, 7=i16, 8=u16, 9=b8, 10=b16, 11=b32, 12=b64 */
    } int_;
    struct {
      double float_val;
      uint8_t float_suffix; /* 0=f64, 1=f32, 2=f16 */
    } float_;
    size_t str_idx;
    int bool_val;
    uint8_t char_val;
    char var_name[128];
    struct {
      OlExpr *inner;
    } neg;
    struct {
      OlExpr *inner;
    } not_;
    struct {
      OlExpr *inner;
    } bnot;
    struct {
      OlBinOp op;
      OlExpr *left;
      OlExpr *right;
    } binary;
    struct {
      char callee[128];
      OlExpr **args;
      size_t arg_count;
    } call;
    struct {
      OlTypeRef to;
      OlExpr *inner;
    } cast_;
    struct {
      OlTypeRef to;
      OlExpr *inner;
    } reinterpret_;
    struct {
      char name[128];
    } addr;
    struct {
      OlTypeRef elem_ty;
      OlExpr *ptr;
    } load;
    struct {
      OlTypeRef elem_ty;
      OlExpr *ptr;
      OlExpr *val;
    } store;
    struct {
      char base_ty[128];
      char field[128];
      OlExpr *obj; /* optional: NULL means synthetic from ptrbind context */
    } field;
    struct {
      char arr_ty[128];
      OlExpr *index_expr;
      OlExpr *arr;
    } index_;
  } u;
};

typedef enum OlStmtKind {
  OL_ST_BLOCK = 1,
  OL_ST_LET,
  OL_ST_ASSIGN,
  OL_ST_IF,
  OL_ST_WHILE,
  OL_ST_RETURN,
  OL_ST_EXPR,
  OL_ST_BREAK,
  OL_ST_CONTINUE,
  OL_ST_PTRBIND,
  OL_ST_DEREF
} OlStmtKind;

typedef struct OlStmt OlStmt;
struct OlStmt {
  OlStmtKind kind;
  int line;
  OlStmt *next; /* within block */
  union {
    struct {
      OlStmt *first;
    } block;
    struct {
      char name[128];
      OlTypeRef ty;
      OlExpr *init;
    } let_;
    struct {
      OlExpr *lhs;  /* Left value: can be VAR, INDEX, FIELD */
      OlExpr *rhs;
    } assign_;
    struct {
      OlExpr *cond;
      OlStmt *then_arm;
      OlStmt *else_arm;
    } if_;
    struct {
      OlExpr *cond;
      OlStmt *body;
    } while_;
    struct {
      OlExpr *val; /* NULL for void return */
    } ret;
    OlExpr *expr;
    struct {
      char bind[128];
      OlTypeRef ty;
      char symbol[128];
    } ptrbind;
    struct {
      char bind[128];
      OlTypeRef ty;
    } deref;
  } u;
};

typedef struct OlFuncDef {
  char name[128];
  char link_name[128]; /* sema: export => name; local fn => __ol_L_<cuhash>_name (unique per .ol) */
  OlTypeRef ret;
  OlParam *params;
  size_t param_count;
  int is_export; /* 1: extern fn ... { } — global symbol uses link_name (= name); 0: fn — local symbol */
  OlStmt *body; /* block */
} OlFuncDef;

typedef enum OlGlobalSection {
  OL_GSEC_DEFAULT = 0,
  OL_GSEC_DATA,
  OL_GSEC_BSS,
  OL_GSEC_RODATA,
  OL_GSEC_CUSTOM
} OlGlobalSection;

typedef struct OlGlobalDef {
  char name[128];
  OlTypeRef ty;
  OlExpr *init; /* NULL => .bss (or zero .data if @data) */
  OlGlobalSection section;
  char custom_section[128]; /* when section == OL_GSEC_CUSTOM, e.g. ".mydata" */
} OlGlobalDef;

typedef struct OlStringLit {
  uint8_t *bytes;
  size_t len;
  char sym[128];
} OlStringLit;

#endif
