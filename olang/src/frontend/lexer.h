#ifndef OLANG_FRONTEND_LEXER_H
#define OLANG_FRONTEND_LEXER_H

#include <stddef.h>
#include <stdint.h>

typedef enum OlTok {
  TOK_EOF = 0,
  TOK_IDENT,
  TOK_INT,
  TOK_FLOAT,
  TOK_STR,
  TOK_CHAR,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_LBRACE,
  TOK_RBRACE,
  TOK_LBRACK,
  TOK_RBRACK,
  TOK_COMMA,
  TOK_SEMI,
  TOK_COLON,
  TOK_AT,
  TOK_DOT,
  TOK_LT,
  TOK_GT,
  TOK_EQ,
  TOK_EQEQ,
  TOK_NE,
  TOK_LE,
  TOK_GE,
  TOK_PLUS,
  TOK_MINUS,
  TOK_STAR,
  TOK_SLASH,
  TOK_PERCENT,
  TOK_AMP,
  TOK_AMPAMP,
  TOK_PIPE,
  TOK_PIPEPIPE,
  TOK_CARET,
  TOK_TILDE,
  TOK_SHL,
  TOK_SHR,
  TOK_BANG,
  /* keywords */
  TOK_KW_EXTERN,
  TOK_KW_LET,
  TOK_KW_IF,
  TOK_KW_ELSE,
  TOK_KW_WHILE,
  TOK_KW_RETURN,
  TOK_KW_TYPE,
  TOK_KW_STRUCT,
  TOK_KW_ARRAY,
  TOK_KW_CAST,
  TOK_KW_REINTERPRET,
  TOK_KW_LOAD,
  TOK_KW_STORE,
  TOK_KW_ADDR,
  TOK_KW_DEREF,
  TOK_KW_AS,
  TOK_KW_FROM,
  TOK_KW_BREAK,
  TOK_KW_CONTINUE,
  TOK_KW_TRUE,
  TOK_KW_FALSE,
  TOK_KW_BOOL,
  TOK_KW_I8,
  TOK_KW_I16,
  TOK_KW_I32,
  TOK_KW_I64,
  TOK_KW_U8,
  TOK_KW_U16,
  TOK_KW_U32,
  TOK_KW_U64,
  TOK_KW_F16,
  TOK_KW_F32,
  TOK_KW_F64,
  TOK_KW_B8,
  TOK_KW_B16,
  TOK_KW_B32,
  TOK_KW_B64,
  TOK_KW_PTR,
  TOK_KW_VOID
} OlTok;

typedef struct OlLexer {
  const char *path;
  const char *src;
  size_t len;
  size_t pos;
  int line;
  OlTok tok;
  char ident[128];
  int64_t int_val;
  uint8_t int_suffix; /* 0=none, 1=i32, 2=i64, 3=u8, 4=u32, 5=u64, 6=i8, 7=i16, 8=u16, 9=b8, 10=b16, 11=b32, 12=b64 */
  double float_val;
  uint8_t float_suffix; /* 0=f64, 1=f32, 2=f16 */
  uint8_t char_val;
  char *str_val; /* unescaped string content, malloc'd per TOK_STR */
  size_t str_len;
  int lexer_error; /* set when tokenization fails (e.g. overlong identifier) */
  char errmsg[256];
} OlLexer;

void ol_lexer_init(OlLexer *L, const char *path, const char *src, size_t len);
void ol_lexer_free_strings(OlLexer *L);
void ol_lexer_next(OlLexer *L);

#endif
