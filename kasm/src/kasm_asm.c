#include "kasm_asm.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim(char *s) {
  char *e;
  while (*s && isspace((unsigned char)*s)) s++;
  e = s + strlen(s);
  while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
  return s;
}

static void strip_line_comment(char *s) {
  for (; *s; ++s) {
    if (*s == '"') {
      ++s;
      while (*s && *s != '"') {
        if (*s == '\\' && s[1]) ++s;
        ++s;
      }
      if (!*s) return;
    } else if (*s == '#') {
      *s = '\0';
      return;
    }
  }
}

static int parse_hex_token(const char *t, uint8_t *out) {
  char *end = NULL;
  long v;
  errno = 0;
  v = strtol(t, &end, 16);
  if (!t[0] || (end && *end)) return 0;
  if (errno == ERANGE || v < 0 || v > 255) return 0;
  *out = (uint8_t)v;
  return 1;
}

static int rest_is_empty_or_comment(const char *rest) {
  if (!rest) return 1;
  while (*rest && isspace((unsigned char)*rest)) rest++;
  return !*rest || *rest == '#';
}

#define KASM_MAX_SECTIONS_FWD 8

typedef struct KasmSection {
  char name[64];
  uint32_t align;
  uint32_t flags;
  uint8_t *data;
  size_t data_len;
} KasmSection;

static int kasm_parse_global_extern(const char *p, const char *kw, char *name, size_t name_sz, unsigned line_no,
                                    KasmSection *cur_sec, int cur_sec_idx, OobjObject *obj, int is_global) {
  int nch = 0;
  const char *rest;
  (void)name_sz;
  if (sscanf(p, "%*[^ ] %127s%n", name, &nch) != 1) {
    fprintf(stderr, "kasm: bad %s syntax (line %u)\n", kw, line_no);
    return -1;
  }
  rest = p + (size_t)nch;
  if (!rest_is_empty_or_comment(rest)) {
    fprintf(stderr, "kasm: trailing junk after %s (line %u)\n", kw, line_no);
    return -1;
  }
  if (is_global) {
    if (!oobj_append_symbol(obj, name, cur_sec_idx, cur_sec->data_len, 1)) {
      fprintf(stderr, "kasm: oom symbol\n");
      return -2;
    }
  } else {
    if (oobj_find_symbol(obj, name) < 0 && !oobj_append_symbol(obj, name, -1, 0, 1)) {
      fprintf(stderr, "kasm: oom extern\n");
      return -2;
    }
  }
  return 0;
}

static int emit_u32le(uint8_t **text, size_t *text_len, uint32_t v) {
  uint8_t *next = (uint8_t *)realloc(*text, *text_len + 4);
  if (!next) return 0;
  *text = next;
  (*text)[*text_len + 0] = (uint8_t)(v & 0xff);
  (*text)[*text_len + 1] = (uint8_t)((v >> 8) & 0xff);
  (*text)[*text_len + 2] = (uint8_t)((v >> 16) & 0xff);
  (*text)[*text_len + 3] = (uint8_t)((v >> 24) & 0xff);
  *text_len += 4;
  return 1;
}

static int emit_bytes(uint8_t **text, size_t *text_len, const uint8_t *bytes, size_t byte_len) {
  uint8_t *next = (uint8_t *)realloc(*text, *text_len + byte_len);
  if (!next) return 0;
  *text = next;
  memcpy(*text + *text_len, bytes, byte_len);
  *text_len += byte_len;
  return 1;
}

#define KASM_MAX_EQUS 256

typedef struct KasmEqu {
  char name[128];
  int64_t value;
} KasmEqu;

static int equ_find(const KasmEqu *equs, int nequ, const char *name) {
  int i;
  for (i = 0; i < nequ; ++i) {
    if (strcmp(equs[i].name, name) == 0) return i;
  }
  return -1;
}

static int equ_substitute(const KasmEqu *equs, int nequ, char *arg) {
  int i = equ_find(equs, nequ, arg);
  if (i < 0) return 0;
  snprintf(arg, 128, "%lld", (long long)equs[i].value);
  return 1;
}

#define KASM_MAX_SECTIONS 8

static KasmSection *find_or_add_section(KasmSection *secs, int *nsec, const char *name, uint32_t align, uint32_t flags) {
  int i;
  for (i = 0; i < *nsec; ++i) {
    if (strcmp(secs[i].name, name) == 0) return &secs[i];
  }
  if (*nsec >= KASM_MAX_SECTIONS) return NULL;
  memset(&secs[*nsec], 0, sizeof(KasmSection));
  snprintf(secs[*nsec].name, sizeof(secs[*nsec].name), "%s", name);
  secs[*nsec].align = align;
  secs[*nsec].flags = flags;
  (*nsec)++;
  return &secs[*nsec - 1];
}

static uint32_t default_section_flags(const char *name) {
  if (strcmp(name, ".text") == 0) return 5;
  if (strcmp(name, ".rodata") == 0) return 4;
  if (strcmp(name, ".data") == 0) return 6;
  if (strcmp(name, ".bss") == 0) return 6;
  return 6;
}

int kasm_compile(IsaSpec *isa, const char *input_path, const char *output_path, OobjObject *obj) {
  KasmSection secs[KASM_MAX_SECTIONS];
  int nsec = 0;
  KasmSection *cur_sec;
  KasmEqu equs[KASM_MAX_EQUS];
  int nequ = 0;
  FILE *f = NULL;
  char line[4096];
  char err[512] = {0};
  int rc = 0;
  int si;

  memset(secs, 0, sizeof(secs));
  cur_sec = find_or_add_section(secs, &nsec, ".text", 16, 5);

  f = fopen(input_path, "rb");
  if (!f) {
    fprintf(stderr, "kasm: cannot open %s\n", input_path);
    return 5;
  }

  {
    unsigned line_no = 0;
    while (fgets(line, sizeof(line), f)) {
      char *p;
      ++line_no;
      if (strlen(line) == sizeof(line) - 1 && !strchr(line, '\n')) {
        fprintf(stderr, "kasm: error: line %u too long (max %zu chars)\n", line_no, sizeof(line) - 1u);
        rc = 5;
        goto asm_done;
      }
      strip_line_comment(line);
      p = trim(line);
      if (!p[0]) continue;
      if (strncmp(p, ".global", 7) == 0 && (p[7] == '\0' || isspace((unsigned char)p[7]))) {
        char name[128];
        int cur_sec_idx = (int)(cur_sec - secs);
        int r = kasm_parse_global_extern(p, ".global", name, sizeof(name), line_no, cur_sec, cur_sec_idx, obj, 1);
        if (r == -1) {
          rc = 5;
          goto asm_done;
        }
        if (r == -2) {
          rc = 4;
          goto asm_done;
        }
      } else if (strncmp(p, ".extern", 7) == 0 && (p[7] == '\0' || isspace((unsigned char)p[7]))) {
        char name[128];
        int r = kasm_parse_global_extern(p, ".extern", name, sizeof(name), line_no, cur_sec, (int)(cur_sec - secs), obj, 0);
        if (r == -1) {
          rc = 5;
          goto asm_done;
        }
        if (r == -2) {
          rc = 4;
          goto asm_done;
        }
      } else if (strncmp(p, ".bytes", 6) == 0 && (p[6] == ' ' || p[6] == '\t')) {
        char *tok = strtok(p + 6, " \t\r\n");
        if (!tok) {
          fprintf(stderr, "kasm: .bytes requires at least one hex byte (line %u)\n", line_no);
          rc = 5;
          goto asm_done;
        }
        while (tok) {
          uint8_t b = 0;
          uint8_t *next;
          if (!parse_hex_token(tok, &b)) {
            fprintf(stderr, "kasm: bad hex token %s (line %u)\n", tok, line_no);
            rc = 5;
            goto asm_done;
          }
          next = (uint8_t *)realloc(cur_sec->data, cur_sec->data_len + 1);
          if (!next) {
            fprintf(stderr, "kasm: oom text\n");
            rc = 4;
            goto asm_done;
          }
          cur_sec->data = next;
          cur_sec->data[cur_sec->data_len++] = b;
          tok = strtok(NULL, " \t\r\n");
        }
      } else if (strchr(p, ':') && p[strlen(p) - 1] == ':') {
        char name[128];
        size_t n = strlen(p);
        if (n > sizeof(name)) {
          fprintf(stderr, "kasm: label too long (max %zu chars) (line %u)\n", sizeof(name) - 1u, line_no);
          rc = 5;
          goto asm_done;
        }
        if (n > 1) {
          memcpy(name, p, n - 1);
          name[n - 1] = '\0';
          {
            int existing = oobj_find_symbol(obj, name);
            if (existing >= 0) {
              int sec_idx = (int)(cur_sec - secs);
              if (obj->symbols[existing].section_index == sec_idx && obj->symbols[existing].value == (uint64_t)cur_sec->data_len) {
                goto label_skip;
              }
              if (obj->symbols[existing].section_index >= 0) {
                fprintf(stderr, "kasm: duplicate label \"%s\" (line %u)\n", name, line_no);
                rc = 5;
                goto asm_done;
              }
              obj->symbols[existing].section_index = sec_idx;
              obj->symbols[existing].value = (uint64_t)cur_sec->data_len;
              goto label_skip;
            }
          }
          if (!oobj_append_symbol(obj, name, (int)(cur_sec - secs), cur_sec->data_len, 1)) {
            rc = 4;
            goto asm_done;
          }
          label_skip: ;
        }
      } else if (strncmp(p, "inst ", 5) == 0) {
        char mnem[64];
        char arg[128];
        int has_arg = 0;
        int nch_inst = 0;
        IsaInstr *ii;
        if (sscanf(p, "inst %63s %127s%n", mnem, arg, &nch_inst) == 2) {
          has_arg = 1;
          if (!rest_is_empty_or_comment(p + nch_inst)) {
            fprintf(stderr, "kasm: trailing junk after inst operand (line %u)\n", line_no);
            rc = 5;
            goto asm_done;
          }
        } else if (sscanf(p, "inst %63s%n", mnem, &nch_inst) == 1) {
          has_arg = 0;
          if (!rest_is_empty_or_comment(p + nch_inst)) {
            fprintf(stderr, "kasm: trailing junk after inst mnemonic (line %u)\n", line_no);
            rc = 5;
            goto asm_done;
          }
        } else {
          fprintf(stderr, "kasm: bad inst syntax (line %u)\n", line_no);
          rc = 5;
          goto asm_done;
        }
        if (has_arg) equ_substitute(equs, nequ, arg);
        ii = isa_find(isa, mnem);
        if (!ii) {
          fprintf(stderr, "kasm: unknown mnemonic %s (line %u)\n", mnem, line_no);
          rc = 5;
          goto asm_done;
        }
        if (ii->reloc_kind == ISA_RELOC_PC32) {
          const char *sym = arg;
          uint64_t reloc_off;
          if (!has_arg) {
            fprintf(stderr, "kasm: %s needs a symbol operand (line %u)\n", mnem, line_no);
            rc = 5;
            goto asm_done;
          }
          if (!emit_bytes(&cur_sec->data, &cur_sec->data_len, ii->bytes, ii->byte_len)) {
            rc = 4;
            goto asm_done;
          }
          reloc_off = (uint64_t)cur_sec->data_len;
          if (!emit_u32le(&cur_sec->data, &cur_sec->data_len, 0)) {
            rc = 4;
            goto asm_done;
          }
          if (!oobj_append_reloc(obj, (uint32_t)(cur_sec - secs), reloc_off, sym, OOBJ_RELOC_PC32, 0)) {
            rc = 4;
            goto asm_done;
          }
          if (oobj_find_symbol(obj, sym) < 0 && !oobj_append_symbol(obj, sym, -1, 0, 1)) {
            rc = 4;
            goto asm_done;
          }
        } else {
          char *end = NULL;
          long imm_l = 0;
          uint64_t imm64 = 0;
          if (!emit_bytes(&cur_sec->data, &cur_sec->data_len, ii->bytes, ii->byte_len)) {
            rc = 4;
            goto asm_done;
          }
          if (ii->imm_bits == 32) {
            int32_t imm32;
            if (!has_arg) {
              fprintf(stderr, "kasm: %s needs an immediate (line %u)\n", mnem, line_no);
              rc = 5;
              goto asm_done;
            }
            errno = 0;
            imm_l = strtol(arg, &end, 0);
            if (end == arg || *end) {
              fprintf(stderr, "kasm: bad immediate for %s (line %u)\n", mnem, line_no);
              rc = 5;
              goto asm_done;
            }
            if (errno == ERANGE || imm_l < (long)INT32_MIN || imm_l > (long)INT32_MAX) {
              fprintf(stderr, "kasm: immediate out of range for int32 (line %u)\n", line_no);
              rc = 5;
              goto asm_done;
            }
            imm32 = (int32_t)imm_l;
            if (!emit_u32le(&cur_sec->data, &cur_sec->data_len, (uint32_t)imm32)) {
              rc = 4;
              goto asm_done;
            }
          } else if (ii->imm_bits == 8) {
            uint8_t *next;
            if (!has_arg) {
              fprintf(stderr, "kasm: %s needs an immediate (line %u)\n", mnem, line_no);
              rc = 5;
              goto asm_done;
            }
            errno = 0;
            imm_l = strtol(arg, &end, 0);
            if (end == arg || *end) {
              fprintf(stderr, "kasm: bad immediate for %s (line %u)\n", mnem, line_no);
              rc = 5;
              goto asm_done;
            }
            if (errno == ERANGE || imm_l < 0 || imm_l > 255) {
              fprintf(stderr, "kasm: immediate must be 0..255 for imm8 (line %u)\n", line_no);
              rc = 5;
              goto asm_done;
            }
            next = (uint8_t *)realloc(cur_sec->data, cur_sec->data_len + 1);
            if (!next) {
              rc = 4;
              goto asm_done;
            }
            cur_sec->data = next;
            cur_sec->data[cur_sec->data_len++] = (uint8_t)imm_l;
          } else if (ii->imm_bits == 64) {
            uint8_t *next;
            if (!has_arg) {
              fprintf(stderr, "kasm: %s needs an immediate (line %u)\n", mnem, line_no);
              rc = 5;
              goto asm_done;
            }
            errno = 0;
            imm64 = strtoull(arg, &end, 0);
            if (end == arg || *end) {
              fprintf(stderr, "kasm: bad immediate for %s (line %u)\n", mnem, line_no);
              rc = 5;
              goto asm_done;
            }
            if (errno == ERANGE) {
              fprintf(stderr, "kasm: immediate out of range for uint64 (line %u)\n", line_no);
              rc = 5;
              goto asm_done;
            }
            next = (uint8_t *)realloc(cur_sec->data, cur_sec->data_len + 8);
            if (!next) {
              rc = 4;
              goto asm_done;
            }
            cur_sec->data = next;
            cur_sec->data[cur_sec->data_len + 0] = (uint8_t)(imm64 & 0xFF);
            cur_sec->data[cur_sec->data_len + 1] = (uint8_t)((imm64 >> 8) & 0xFF);
            cur_sec->data[cur_sec->data_len + 2] = (uint8_t)((imm64 >> 16) & 0xFF);
            cur_sec->data[cur_sec->data_len + 3] = (uint8_t)((imm64 >> 24) & 0xFF);
            cur_sec->data[cur_sec->data_len + 4] = (uint8_t)((imm64 >> 32) & 0xFF);
            cur_sec->data[cur_sec->data_len + 5] = (uint8_t)((imm64 >> 40) & 0xFF);
            cur_sec->data[cur_sec->data_len + 6] = (uint8_t)((imm64 >> 48) & 0xFF);
            cur_sec->data[cur_sec->data_len + 7] = (uint8_t)((imm64 >> 56) & 0xFF);
            cur_sec->data_len += 8;
          } else if (has_arg) {
            fprintf(stderr, "kasm: mnemonic %s does not accept immediate (line %u)\n", mnem, line_no);
            rc = 5;
            goto asm_done;
          }
        }
      } else if (strncmp(p, ".equ", 4) == 0 && (p[4] == ' ' || p[4] == '\t')) {
        char equ_name[128];
        char equ_val_str[128];
        char *endp;
        long long equ_val;
        if (sscanf(p + 4, " %127[^, \t] , %127s", equ_name, equ_val_str) != 2 &&
            sscanf(p + 4, " %127s %127s", equ_name, equ_val_str) != 2) {
          fprintf(stderr, "kasm: bad .equ syntax, expected '.equ name, value' (line %u)\n", line_no);
          rc = 5;
          goto asm_done;
        }
        if (equ_find(equs, nequ, equ_name) >= 0) {
          fprintf(stderr, "kasm: duplicate .equ \"%s\" (line %u)\n", equ_name, line_no);
          rc = 5;
          goto asm_done;
        }
        if (nequ >= KASM_MAX_EQUS) {
          fprintf(stderr, "kasm: too many .equ definitions (max %d)\n", KASM_MAX_EQUS);
          rc = 5;
          goto asm_done;
        }
        errno = 0;
        equ_val = strtoll(equ_val_str, &endp, 0);
        if (endp == equ_val_str || *endp) {
          fprintf(stderr, "kasm: bad .equ value \"%s\" (line %u)\n", equ_val_str, line_no);
          rc = 5;
          goto asm_done;
        }
        if (errno == ERANGE) {
          fprintf(stderr, "kasm: .equ value out of range (line %u)\n", line_no);
          rc = 5;
          goto asm_done;
        }
        snprintf(equs[nequ].name, sizeof(equs[nequ].name), "%s", equ_name);
        equs[nequ].value = (int64_t)equ_val;
        nequ++;
      } else if (strncmp(p, ".align", 6) == 0 && (p[6] == ' ' || p[6] == '\t')) {
        unsigned align_val = 0;
        size_t pad;
        if (sscanf(p + 6, " %u", &align_val) != 1 || align_val == 0) {
          fprintf(stderr, "kasm: .align requires a positive integer (line %u)\n", line_no);
          rc = 5;
          goto asm_done;
        }
        pad = (align_val - (cur_sec->data_len % align_val)) % align_val;
        if (pad > 0) {
          uint8_t *next = (uint8_t *)realloc(cur_sec->data, cur_sec->data_len + pad);
          if (!next) { rc = 4; goto asm_done; }
          cur_sec->data = next;
          memset(cur_sec->data + cur_sec->data_len, 0, pad);
          cur_sec->data_len += pad;
        }
      } else if (strncmp(p, ".section", 8) == 0 && (p[8] == ' ' || p[8] == '\t')) {
        char secname[64];
        if (sscanf(p + 8, " %63s", secname) != 1) {
          fprintf(stderr, "kasm: .section requires a name (line %u)\n", line_no);
          rc = 5;
          goto asm_done;
        }
        cur_sec = find_or_add_section(secs, &nsec, secname, 8, default_section_flags(secname));
        if (!cur_sec) {
          fprintf(stderr, "kasm: too many sections (max %d) (line %u)\n", KASM_MAX_SECTIONS, line_no);
          rc = 5;
          goto asm_done;
        }
      } else {
        fprintf(stderr, "kasm: unknown directive or syntax (line %u): %s\n", line_no, p);
        rc = 5;
        goto asm_done;
      }
    }
  }

asm_done:
  fclose(f);
  if (rc != 0) {
    for (si = 0; si < nsec; ++si) free(secs[si].data);
    return rc;
  }

  for (si = 0; si < nsec; ++si) {
    if (!oobj_append_section(obj, secs[si].name, secs[si].align, secs[si].flags, secs[si].data, secs[si].data_len)) {
      for (si = 0; si < nsec; ++si) free(secs[si].data);
      fprintf(stderr, "kasm: oom section\n");
      return 4;
    }
  }
  for (si = 0; si < nsec; ++si) free(secs[si].data);

  if (!oobj_write_file(output_path, obj, err, sizeof(err))) {
    fprintf(stderr, "kasm: %s\n", err);
    return 6;
  }
  return 0;
}
