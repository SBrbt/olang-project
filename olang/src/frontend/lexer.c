#include "lexer.h"

#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Must be < sizeof(OlLexer::ident) which is 128. */
enum { OL_MAX_IDENT_LEN = 63 };
enum { OL_MAX_STR_LIT_BYTES = 1024 * 1024 };

static void skip_ws(OlLexer *L) {
  while (L->pos < L->len) {
    char c = L->src[L->pos];
    if (c == ' ' || c == '\t' || c == '\r') {
      L->pos++;
      continue;
    }
    if (c == '\n') {
      L->pos++;
      L->line++;
      continue;
    }
    if (c == '/' && L->pos + 1 < L->len && L->src[L->pos + 1] == '/') {
      L->pos += 2;
      while (L->pos < L->len && L->src[L->pos] != '\n') L->pos++;
      continue;
    }
    break;
  }
}

static int kw_tok(const char *s) {
  if (strcmp(s, "extern") == 0) return TOK_KW_EXTERN;
  if (strcmp(s, "fn") == 0) return TOK_KW_FN;
  if (strcmp(s, "let") == 0) return TOK_KW_LET;
  if (strcmp(s, "if") == 0) return TOK_KW_IF;
  if (strcmp(s, "else") == 0) return TOK_KW_ELSE;
  if (strcmp(s, "while") == 0) return TOK_KW_WHILE;
  if (strcmp(s, "return") == 0) return TOK_KW_RETURN;
  if (strcmp(s, "type") == 0) return TOK_KW_TYPE;
  if (strcmp(s, "struct") == 0) return TOK_KW_STRUCT;
  if (strcmp(s, "array") == 0) return TOK_KW_ARRAY;
  if (strcmp(s, "cast") == 0) return TOK_KW_CAST;
  if (strcmp(s, "reinterpret") == 0) return TOK_KW_REINTERPRET;
  if (strcmp(s, "load") == 0) return TOK_KW_LOAD;
  if (strcmp(s, "store") == 0) return TOK_KW_STORE;
  if (strcmp(s, "addr") == 0) return TOK_KW_ADDR;
  if (strcmp(s, "deref") == 0) return TOK_KW_DEREF;
  if (strcmp(s, "as") == 0) return TOK_KW_AS;
  if (strcmp(s, "from") == 0) return TOK_KW_FROM;
  if (strcmp(s, "break") == 0) return TOK_KW_BREAK;
  if (strcmp(s, "continue") == 0) return TOK_KW_CONTINUE;
  if (strcmp(s, "true") == 0) return TOK_KW_TRUE;
  if (strcmp(s, "false") == 0) return TOK_KW_FALSE;
  if (strcmp(s, "bool") == 0) return TOK_KW_BOOL;
  if (strcmp(s, "i8") == 0) return TOK_KW_I8;
  if (strcmp(s, "i16") == 0) return TOK_KW_I16;
  if (strcmp(s, "i32") == 0) return TOK_KW_I32;
  if (strcmp(s, "i64") == 0) return TOK_KW_I64;
  if (strcmp(s, "u8") == 0) return TOK_KW_U8;
  if (strcmp(s, "u16") == 0) return TOK_KW_U16;
  if (strcmp(s, "u32") == 0) return TOK_KW_U32;
  if (strcmp(s, "u64") == 0) return TOK_KW_U64;
  if (strcmp(s, "f16") == 0) return TOK_KW_F16;
  if (strcmp(s, "f32") == 0) return TOK_KW_F32;
  if (strcmp(s, "f64") == 0) return TOK_KW_F64;
  if (strcmp(s, "b8") == 0) return TOK_KW_B8;
  if (strcmp(s, "b16") == 0) return TOK_KW_B16;
  if (strcmp(s, "b32") == 0) return TOK_KW_B32;
  if (strcmp(s, "b64") == 0) return TOK_KW_B64;
  if (strcmp(s, "ptr") == 0) return TOK_KW_PTR;
  if (strcmp(s, "void") == 0) return TOK_KW_VOID;
  return TOK_IDENT;
}

void ol_lexer_init(OlLexer *L, const char *path, const char *src, size_t len) {
  memset(L, 0, sizeof(*L));
  L->path = path;
  L->src = src;
  L->len = len;
  L->line = 1;
  L->tok = TOK_EOF;
}

void ol_lexer_free_strings(OlLexer *L) {
  free(L->str_val);
  L->str_val = NULL;
  L->str_len = 0;
}

void ol_lexer_next(OlLexer *L) {
  char c;
  free(L->str_val);
  L->str_val = NULL;
  L->str_len = 0;
  L->int_suffix = 0;
  L->float_val = 0.0;
  L->float_suffix = 0;
  L->lexer_error = 0;
  L->errmsg[0] = '\0';
  skip_ws(L);
  if (L->pos >= L->len) {
    L->tok = TOK_EOF;
    return;
  }
  c = L->src[L->pos];
  if (isalpha((unsigned char)c) || c == '_') {
    size_t start = L->pos;
    while (L->pos < L->len && (isalnum((unsigned char)L->src[L->pos]) || L->src[L->pos] == '_')) L->pos++;
    {
      size_t n = L->pos - start;
      if (n > (size_t)OL_MAX_IDENT_LEN) {
        L->tok = TOK_EOF;
        L->lexer_error = 1;
        snprintf(L->errmsg, sizeof(L->errmsg), "identifier too long (max %d characters)", OL_MAX_IDENT_LEN);
        return;
      }
      memcpy(L->ident, L->src + start, n);
      L->ident[n] = '\0';
    }
    L->tok = kw_tok(L->ident);
    return;
  }
  if (isdigit((unsigned char)c)) {
    uint64_t v = 0;
    int overflow = 0;
    int is_non_decimal = 0;
    size_t int_end;
    L->int_suffix = 0;
    if (c == '0' && L->pos + 1 < L->len && (L->src[L->pos + 1] == 'x' || L->src[L->pos + 1] == 'X')) {
      is_non_decimal = 1;
      L->pos += 2;
      while (L->pos < L->len) {
        char h = L->src[L->pos];
        unsigned dig;
        if (h >= '0' && h <= '9') dig = (unsigned)(h - '0');
        else if (h >= 'a' && h <= 'f') dig = (unsigned)(h - 'a' + 10);
        else if (h >= 'A' && h <= 'F') dig = (unsigned)(h - 'A' + 10);
        else break;
        if (v > UINT64_MAX / 16u) overflow = 1;
        v = v * 16u + dig;
        L->pos++;
      }
      L->int_val = (int64_t)v;
    } else if (c == '0' && L->pos + 1 < L->len && (L->src[L->pos + 1] == 'b' || L->src[L->pos + 1] == 'B')) {
      is_non_decimal = 1;
      L->pos += 2;
      while (L->pos < L->len && (L->src[L->pos] == '0' || L->src[L->pos] == '1')) {
        unsigned dig = (unsigned)(L->src[L->pos] - '0');
        if (v > UINT64_MAX / 2u) overflow = 1;
        v = v * 2u + dig;
        L->pos++;
      }
      L->int_val = (int64_t)v;
    } else if (c == '0' && L->pos + 1 < L->len && (L->src[L->pos + 1] == 'o' || L->src[L->pos + 1] == 'O')) {
      is_non_decimal = 1;
      L->pos += 2;
      while (L->pos < L->len && L->src[L->pos] >= '0' && L->src[L->pos] <= '7') {
        unsigned dig = (unsigned)(L->src[L->pos] - '0');
        if (v > UINT64_MAX / 8u) overflow = 1;
        v = v * 8u + dig;
        L->pos++;
      }
      L->int_val = (int64_t)v;
    } else {
      while (L->pos < L->len && isdigit((unsigned char)L->src[L->pos])) {
        unsigned dig = (unsigned)(L->src[L->pos] - '0');
        if (v > (UINT64_MAX - dig) / 10u) overflow = 1;
        v = v * 10u + dig;
        L->pos++;
      }
      L->int_val = (int64_t)v;
    }
    int_end = L->pos;
    /* Check for float literal: decimal digits followed by '.' + digit, or 'e'/'E' */
    if (!is_non_decimal &&
        ((L->pos < L->len && L->src[L->pos] == '.' && L->pos + 1 < L->len && isdigit((unsigned char)L->src[L->pos + 1])) ||
         (L->pos < L->len && (L->src[L->pos] == 'e' || L->src[L->pos] == 'E')))) {
      size_t fstart = int_end;
      char *fend = NULL;
      /* Rewind to start of integer part to let strtod parse the whole number */
      while (fstart > 0 && (isdigit((unsigned char)L->src[fstart - 1]))) fstart--;
      L->float_val = strtod(L->src + fstart, &fend);
      if (fend && fend > L->src + int_end) {
        L->pos = (size_t)(fend - L->src);
      }
      /* Check for float suffix: f16, f32, f64 */
      {
        size_t left = L->len - L->pos;
        if (left >= 3 && L->src[L->pos] == 'f' && L->src[L->pos+1] == '6' && L->src[L->pos+2] == '4') {
          size_t after = L->pos + 3;
          if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->float_suffix = 0; }
        } else if (left >= 3 && L->src[L->pos] == 'f' && L->src[L->pos+1] == '3' && L->src[L->pos+2] == '2') {
          size_t after = L->pos + 3;
          if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->float_suffix = 1; }
        } else if (left >= 3 && L->src[L->pos] == 'f' && L->src[L->pos+1] == '1' && L->src[L->pos+2] == '6') {
          size_t after = L->pos + 3;
          if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->float_suffix = 2; }
        }
      }
      L->tok = TOK_FLOAT;
      return;
    }
    {
      /* suffix: i8, i16, i32, i64, u8, u16, u32, u64 */
      size_t left = L->len - L->pos;
      if (left >= 2 && L->src[L->pos] == 'i' && L->src[L->pos+1] == '8') {
        size_t after = L->pos + 2;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 6; }
      } else if (left >= 3 && L->src[L->pos] == 'i' && L->src[L->pos+1] == '1' && L->src[L->pos+2] == '6') {
        size_t after = L->pos + 3;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 7; }
      } else if (left >= 3 && L->src[L->pos] == 'i' && L->src[L->pos+1] == '3' && L->src[L->pos+2] == '2') {
        size_t after = L->pos + 3;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 1; }
      } else if (left >= 3 && L->src[L->pos] == 'i' && L->src[L->pos+1] == '6' && L->src[L->pos+2] == '4') {
        size_t after = L->pos + 3;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 2; }
      } else if (left >= 2 && L->src[L->pos] == 'u' && L->src[L->pos+1] == '8') {
        size_t after = L->pos + 2;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 3; }
      } else if (left >= 3 && L->src[L->pos] == 'u' && L->src[L->pos+1] == '1' && L->src[L->pos+2] == '6') {
        size_t after = L->pos + 3;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 8; }
      } else if (left >= 3 && L->src[L->pos] == 'u' && L->src[L->pos+1] == '3' && L->src[L->pos+2] == '2') {
        size_t after = L->pos + 3;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 4; }
      } else if (left >= 3 && L->src[L->pos] == 'u' && L->src[L->pos+1] == '6' && L->src[L->pos+2] == '4') {
        size_t after = L->pos + 3;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 5; }
      } else if (left >= 2 && L->src[L->pos] == 'b' && L->src[L->pos+1] == '8') {
        size_t after = L->pos + 2;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 9; }
      } else if (left >= 3 && L->src[L->pos] == 'b' && L->src[L->pos+1] == '1' && L->src[L->pos+2] == '6') {
        size_t after = L->pos + 3;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 10; }
      } else if (left >= 3 && L->src[L->pos] == 'b' && L->src[L->pos+1] == '3' && L->src[L->pos+2] == '2') {
        size_t after = L->pos + 3;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 11; }
      } else if (left >= 3 && L->src[L->pos] == 'b' && L->src[L->pos+1] == '6' && L->src[L->pos+2] == '4') {
        size_t after = L->pos + 3;
        if (after >= L->len || !(isalnum((unsigned char)L->src[after]) || L->src[after] == '_')) { L->pos = after; L->int_suffix = 12; }
      }
    }
    /* Validate range based on suffix */
    if (!overflow) {
      if (L->int_suffix == 5 || L->int_suffix == 12) { /* u64 / b64 */
      } else if (L->int_suffix == 4 || L->int_suffix == 11) { /* u32 / b32 */
        if (v > UINT32_MAX) overflow = 1;
      } else if (L->int_suffix == 8 || L->int_suffix == 10) { /* u16 / b16 */
        if (v > 65535) overflow = 1;
      } else if (L->int_suffix == 3 || L->int_suffix == 9) { /* u8 / b8 */
        if (v > 255) overflow = 1;
      } else if (L->int_suffix == 2) { /* i64 */
        if (v > (uint64_t)LLONG_MAX) overflow = 1;
      } else if (L->int_suffix == 1) { /* i32 */
        if (v > INT32_MAX) overflow = 1;
      } else if (L->int_suffix == 7) { /* i16 */
        if (v > 32767) overflow = 1;
      } else if (L->int_suffix == 6) { /* i8 */
        if (v > 127) overflow = 1;
      } else { /* no suffix: default i32 */
        if (v > INT32_MAX) overflow = 1;
      }
    }
    if (overflow) {
      L->tok = TOK_EOF;
      L->lexer_error = 1;
      snprintf(L->errmsg, sizeof(L->errmsg), "integer literal overflow");
      return;
    }
    L->tok = TOK_INT;
    return;
  }
  if (c == '\'') {
    char ch;
    L->pos++;
    if (L->pos >= L->len) { L->tok = TOK_EOF; L->lexer_error = 1; snprintf(L->errmsg, sizeof(L->errmsg), "unterminated char literal"); return; }
    ch = L->src[L->pos++];
    if (ch == '\\' && L->pos < L->len) {
      char e = L->src[L->pos++];
      if (e == 'n') ch = '\n';
      else if (e == 'r') ch = '\r';
      else if (e == 't') ch = '\t';
      else if (e == '\\') ch = '\\';
      else if (e == '\'') ch = '\'';
      else if (e == '0') ch = '\0';
      else ch = e;
    }
    if (L->pos >= L->len || L->src[L->pos] != '\'') { L->tok = TOK_EOF; L->lexer_error = 1; snprintf(L->errmsg, sizeof(L->errmsg), "unterminated char literal"); return; }
    L->pos++;
    L->char_val = (uint8_t)ch;
    L->tok = TOK_CHAR;
    return;
  }
  if (c == '"') {
    size_t cap = 32, out_len = 0;
    char *buf = (char *)malloc(cap);
    L->pos++;
    if (!buf) {
      L->tok = TOK_EOF;
      return;
    }
    while (L->pos < L->len && L->src[L->pos] != '"') {
      char ch = L->src[L->pos++];
      if (ch == '\\' && L->pos < L->len) {
        char e = L->src[L->pos++];
        if (e == 'n') ch = '\n';
        else if (e == 'r') ch = '\r';
        else if (e == 't') ch = '\t';
        else if (e == '\\') ch = '\\';
        else if (e == '"') ch = '"';
        else if (e == '0') ch = '\0';
        else ch = e;
      }
      if (out_len >= (size_t)OL_MAX_STR_LIT_BYTES) {
        free(buf);
        L->tok = TOK_EOF;
        L->lexer_error = 1;
        snprintf(L->errmsg, sizeof(L->errmsg), "string literal too long (max %d bytes)", OL_MAX_STR_LIT_BYTES);
        return;
      }
      if (out_len + 1 >= cap) {
        cap *= 2;
        buf = (char *)realloc(buf, cap);
        if (!buf) {
          L->tok = TOK_EOF;
          return;
        }
      }
      buf[out_len++] = ch;
    }
    if (L->pos < L->len && L->src[L->pos] == '"') {
      L->pos++;
    } else {
      free(buf);
      L->tok = TOK_EOF;
      L->lexer_error = 1;
      snprintf(L->errmsg, sizeof(L->errmsg), "unterminated string literal");
      return;
    }
    buf[out_len] = '\0';
    L->str_val = buf;
    L->str_len = out_len;
    L->tok = TOK_STR;
    return;
  }
  L->pos++;
  switch (c) {
    case '(':
      L->tok = TOK_LPAREN;
      return;
    case ')':
      L->tok = TOK_RPAREN;
      return;
    case '{':
      L->tok = TOK_LBRACE;
      return;
    case '}':
      L->tok = TOK_RBRACE;
      return;
    case '[':
      L->tok = TOK_LBRACK;
      return;
    case ']':
      L->tok = TOK_RBRACK;
      return;
    case ',':
      L->tok = TOK_COMMA;
      return;
    case ';':
      L->tok = TOK_SEMI;
      return;
    case ':':
      L->tok = TOK_COLON;
      return;
    case '.':
      L->tok = TOK_DOT;
      return;
    case '<':
      if (L->pos < L->len && L->src[L->pos] == '<') {
        L->pos++;
        L->tok = TOK_SHL;
      } else if (L->pos < L->len && L->src[L->pos] == '=') {
        L->pos++;
        L->tok = TOK_LE;
      } else
        L->tok = TOK_LT;
      return;
    case '>':
      if (L->pos < L->len && L->src[L->pos] == '>') {
        L->pos++;
        L->tok = TOK_SHR;
      } else if (L->pos < L->len && L->src[L->pos] == '=') {
        L->pos++;
        L->tok = TOK_GE;
      } else
        L->tok = TOK_GT;
      return;
    case '=':
      if (L->pos < L->len && L->src[L->pos] == '=') {
        L->pos++;
        L->tok = TOK_EQEQ;
      } else
        L->tok = TOK_EQ;
      return;
    case '!':
      if (L->pos < L->len && L->src[L->pos] == '=') {
        L->pos++;
        L->tok = TOK_NE;
      } else {
        L->tok = TOK_BANG;
      }
      return;
    case '&':
      if (L->pos < L->len && L->src[L->pos] == '&') {
        L->pos++;
        L->tok = TOK_AMPAMP;
      } else {
        L->tok = TOK_AMP;
      }
      return;
    case '|':
      if (L->pos < L->len && L->src[L->pos] == '|') {
        L->pos++;
        L->tok = TOK_PIPEPIPE;
      } else {
        L->tok = TOK_PIPE;
      }
      return;
    case '^':
      L->tok = TOK_CARET;
      return;
    case '~':
      L->tok = TOK_TILDE;
      return;
    case '+':
      L->tok = TOK_PLUS;
      return;
    case '-':
      L->tok = TOK_MINUS;
      return;
    case '*':
      L->tok = TOK_STAR;
      return;
    case '/':
      L->tok = TOK_SLASH;
      return;
    case '%':
      L->tok = TOK_PERCENT;
      return;
    case '@':
      L->tok = TOK_AT;
      return;
    default:
      L->tok = TOK_EOF;
      return;
  }
}
