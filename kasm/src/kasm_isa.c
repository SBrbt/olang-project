#include "kasm_isa.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_all_text(const char *path, char **out, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  long sz;
  char *buf;
  size_t n;
  if (!f) return 0;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return 0;
  }
  sz = ftell(f);
  if (sz < 0) {
    fclose(f);
    return 0;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return 0;
  }
  buf = (char *)malloc((size_t)sz + 1);
  if (!buf) {
    fclose(f);
    return 0;
  }
  n = fread(buf, 1, (size_t)sz, f);
  fclose(f);
  buf[n] = '\0';
  *out = buf;
  *out_len = n;
  return 1;
}

static const char *skip_ws_c(const char *p) {
  while (p && *p && isspace((unsigned char)*p)) ++p;
  return p;
}

static int json_read_string_after_key(const char *obj, const char *key, char *out, size_t out_sz) {
  const char *k = strstr(obj, key);
  const char *q1;
  const char *q2;
  size_t n;
  if (!k) return 0;
  k = strchr(k, ':');
  if (!k) return 0;
  k = skip_ws_c(k + 1);
  if (!k || *k != '"') return 0;
  q1 = k + 1;
  q2 = q1;
  while (*q2 && *q2 != '"') {
    if (*q2 == '\\' && q2[1]) { q2 += 2; continue; }
    q2++;
  }
  if (!*q2) return 0;
  n = (size_t)(q2 - q1);
  if (n >= out_sz) n = out_sz - 1;
  memcpy(out, q1, n);
  out[n] = '\0';
  return 1;
}

static int json_read_u32_after_key(const char *obj, const char *key, uint32_t *out) {
  const char *k = strstr(obj, key);
  unsigned v = 0;
  if (!k) return 0;
  k = strchr(k, ':');
  if (!k) return 0;
  k = skip_ws_c(k + 1);
  if (!k || !isdigit((unsigned char)*k)) return 0;
  if (sscanf(k, "%u", &v) != 1) return 0;
  *out = (uint32_t)v;
  return 1;
}

static const char *skip_json_str(const char *p) {
  if (*p != '"') return p;
  ++p;
  for (; *p; ++p) {
    if (*p == '\\' && p[1]) { ++p; continue; }
    if (*p == '"') return p;
  }
  return p;
}

static const char *json_object_end(const char *start) {
  int depth = 0;
  const char *q;
  if (!start || *start != '{') return NULL;
  for (q = start; *q; ++q) {
    if (*q == '"') { q = skip_json_str(q); if (!*q) break; continue; }
    if (*q == '{') depth++;
    else if (*q == '}') {
      depth--;
      if (depth == 0) return q;
    }
  }
  return NULL;
}

static int parse_hex_blob(const char *hex, uint8_t **out, size_t *out_len) {
  size_t n = strlen(hex);
  size_t i;
  uint8_t *buf;
  if (n % 2 != 0) return 0;
  buf = (uint8_t *)malloc(n / 2);
  if (!buf && n > 0) return 0;
  for (i = 0; i < n / 2; ++i) {
    char tmp[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
    char *end = NULL;
    long v = strtol(tmp, &end, 16);
    if (end != tmp + 2 || v < 0 || v > 255) {
      free(buf);
      return 0;
    }
    buf[i] = (uint8_t)v;
  }
  *out = buf;
  *out_len = n / 2;
  return 1;
}

void isa_free(IsaSpec *isa) {
  size_t i;
  for (i = 0; i < isa->count; ++i) free(isa->items[i].bytes);
  free(isa->items);
  memset(isa, 0, sizeof(*isa));
}

int isa_load(const char *path, IsaSpec *out, char *err, size_t err_len) {
  char *json = NULL;
  size_t json_len = 0;
  const char *p;
  if (!read_all_text(path, &json, &json_len)) {
    snprintf(err, err_len, "cannot read isa file: %s", path);
    return 0;
  }
  (void)json_len;
  memset(out, 0, sizeof(*out));
  p = json;
  while ((p = strstr(p, "\"mnemonic\"")) != NULL) {
    char mnem[64] = {0};
    char hex[1024] = {0};
    char reloc_str[32] = {0};
    uint32_t imm = 0;
    IsaRelocKind reloc_kind = ISA_RELOC_NONE;
    IsaInstr *next;
    uint8_t *bytes = NULL;
    size_t blen = 0;
    const char *obj_start;
    const char *obj_end;
    const char *t;
    size_t obj_n;
    char *obj_buf;
    t = p;
    while (t > json && *t != '{') t--;
    if (t < json || *t != '{') {
      free(json);
      snprintf(err, err_len, "isa json: no '{' before mnemonic");
      isa_free(out);
      return 0;
    }
    obj_start = t;
    obj_end = json_object_end(obj_start);
    if (!obj_end) {
      free(json);
      snprintf(err, err_len, "isa json: unclosed instruction object");
      isa_free(out);
      return 0;
    }
    obj_n = (size_t)(obj_end - obj_start + 1);
    obj_buf = (char *)malloc(obj_n + 1);
    if (!obj_buf) {
      free(json);
      snprintf(err, err_len, "oom isa json");
      isa_free(out);
      return 0;
    }
    memcpy(obj_buf, obj_start, obj_n);
    obj_buf[obj_n] = '\0';
    if (!json_read_string_after_key(obj_buf, "\"mnemonic\"", mnem, sizeof(mnem)) ||
        !json_read_string_after_key(obj_buf, "\"bytes\"", hex, sizeof(hex))) {
      free(obj_buf);
      free(json);
      snprintf(err, err_len, "isa json: missing mnemonic or bytes in object");
      isa_free(out);
      return 0;
    }
    if (!json_read_u32_after_key(obj_buf, "\"imm_bits\"", &imm)) imm = 0;
    if (json_read_string_after_key(obj_buf, "\"reloc\"", reloc_str, sizeof(reloc_str))) {
      if (strcmp(reloc_str, "pc32") == 0) reloc_kind = ISA_RELOC_PC32;
      else {
        free(obj_buf);
        free(json);
        snprintf(err, err_len, "isa json: unknown reloc \"%s\"", reloc_str);
        isa_free(out);
        return 0;
      }
    }
    free(obj_buf);
    if (reloc_kind == ISA_RELOC_PC32 && imm != 32) {
      free(json);
      snprintf(err, err_len, "isa json: reloc pc32 requires imm_bits 32 (%s)", mnem);
      isa_free(out);
      return 0;
    }
    if (!(imm == 0 || imm == 8 || imm == 32 || imm == 64)) {
      free(json);
      snprintf(err, err_len, "unsupported imm_bits in isa: %u", imm);
      isa_free(out);
      return 0;
    }

    if (!parse_hex_blob(hex, &bytes, &blen)) {
      free(json);
      snprintf(err, err_len, "bad isa bytes for %s", mnem);
      isa_free(out);
      return 0;
    }
    {
      size_t dj;
      for (dj = 0; dj < out->count; ++dj) {
        if (strcmp(out->items[dj].mnemonic, mnem) == 0) {
          free(bytes);
          free(json);
          snprintf(err, err_len, "isa json: duplicate mnemonic \"%s\"", mnem);
          isa_free(out);
          return 0;
        }
      }
    }
    next = (IsaInstr *)realloc(out->items, (out->count + 1) * sizeof(IsaInstr));
    if (!next) {
      free(bytes);
      free(json);
      snprintf(err, err_len, "oom isa");
      isa_free(out);
      return 0;
    }
    out->items = next;
    memset(&out->items[out->count], 0, sizeof(IsaInstr));
    snprintf(out->items[out->count].mnemonic, sizeof(out->items[out->count].mnemonic), "%s", mnem);
    out->items[out->count].bytes = bytes;
    out->items[out->count].byte_len = blen;
    out->items[out->count].imm_bits = imm;
    out->items[out->count].reloc_kind = reloc_kind;
    out->count++;
    p = obj_end + 1;
  }
  free(json);
  if (out->count == 0) {
    snprintf(err, err_len, "isa has no instructions");
    return 0;
  }
  return 1;
}

IsaInstr *isa_find(IsaSpec *isa, const char *mnemonic) {
  size_t i;
  for (i = 0; i < isa->count; ++i) {
    if (strcmp(isa->items[i].mnemonic, mnemonic) == 0) return &isa->items[i];
  }
  return NULL;
}
