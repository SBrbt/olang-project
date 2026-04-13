#include "oobj.h"

#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_s(const char *s) {
  size_t n = strlen(s) + 1;
  char *out = (char *)malloc(n);
  if (!out) return NULL;
  memcpy(out, s, n);
  return out;
}

void oobj_init(OobjObject *obj) {
  if (!obj) return;
  memset(obj, 0, sizeof(*obj));
  snprintf(obj->target, sizeof(obj->target), "%s", "x86_64-unknown-linux-gnu");
}

void oobj_free(OobjObject *obj) {
  size_t i;
  if (!obj) return;
  for (i = 0; i < obj->section_count; ++i) {
    free(obj->sections[i].name);
    free(obj->sections[i].data);
  }
  for (i = 0; i < obj->symbol_count; ++i) {
    free(obj->symbols[i].name);
  }
  for (i = 0; i < obj->reloc_count; ++i) {
    free(obj->relocs[i].symbol_name);
  }
  free(obj->sections);
  free(obj->symbols);
  free(obj->relocs);
  memset(obj, 0, sizeof(*obj));
}

static int hex_nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static int parse_hex_bytes(const char *hex, uint8_t **out, size_t *out_len) {
  size_t n = strlen(hex);
  size_t i, count = 0;
  uint8_t *buf = NULL;
  if (n % 2 != 0) return 0;
  count = n / 2;
  if (count > 0) {
    buf = (uint8_t *)malloc(count);
    if (!buf) return 0;
  }
  for (i = 0; i < count; ++i) {
    int hi = hex_nibble(hex[i * 2]);
    int lo = hex_nibble(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0) {
      free(buf);
      return 0;
    }
    buf[i] = (uint8_t)((hi << 4) | lo);
  }
  *out = buf;
  *out_len = count;
  return 1;
}

static char *bytes_to_hex(const uint8_t *data, size_t len) {
  static const char *lut = "0123456789abcdef";
  char *out;
  size_t i;
  if (len > (SIZE_MAX - 1u) / 2u) return NULL;
  out = (char *)malloc(len * 2 + 1);
  if (!out) return NULL;
  for (i = 0; i < len; ++i) {
    out[i * 2] = lut[(data[i] >> 4) & 0xf];
    out[i * 2 + 1] = lut[data[i] & 0xf];
  }
  out[len * 2] = '\0';
  return out;
}

int oobj_append_section(OobjObject *obj, const char *name, uint32_t align, uint32_t flags, const uint8_t *data, size_t data_len) {
  OobjSection *next;
  OobjSection *s;
  if (obj->section_count > (SIZE_MAX / sizeof(OobjSection)) - 1u) return 0;
  next = (OobjSection *)realloc(obj->sections, (obj->section_count + 1) * sizeof(OobjSection));
  if (!next) return 0;
  obj->sections = next;
  s = &obj->sections[obj->section_count];
  memset(s, 0, sizeof(*s));
  s->name = dup_s(name);
  s->align = align;
  s->flags = flags;
  s->data_len = data_len;
  if (!s->name) return 0;
  if (data_len > 0) {
    s->data = (uint8_t *)malloc(data_len);
    if (!s->data) {
      free(s->name);
      s->name = NULL;
      return 0;
    }
    memcpy(s->data, data, data_len);
  }
  obj->section_count++;
  return 1;
}

int oobj_append_symbol(OobjObject *obj, const char *name, int32_t section_index, uint64_t value, int is_global) {
  OobjSymbol *next;
  OobjSymbol *s;
  if (obj->symbol_count > (SIZE_MAX / sizeof(OobjSymbol)) - 1u) return 0;
  next = (OobjSymbol *)realloc(obj->symbols, (obj->symbol_count + 1) * sizeof(OobjSymbol));
  if (!next) return 0;
  obj->symbols = next;
  s = &obj->symbols[obj->symbol_count];
  memset(s, 0, sizeof(*s));
  s->name = dup_s(name);
  if (!s->name) return 0;
  s->section_index = section_index;
  s->value = value;
  s->is_global = is_global ? 1u : 0u;
  obj->symbol_count++;
  return 1;
}

int oobj_append_reloc(OobjObject *obj, uint32_t section_index, uint64_t offset, const char *symbol_name, OobjRelocType type, int64_t addend) {
  OobjReloc *next;
  OobjReloc *r;
  if (obj->reloc_count > (SIZE_MAX / sizeof(OobjReloc)) - 1u) return 0;
  next = (OobjReloc *)realloc(obj->relocs, (obj->reloc_count + 1) * sizeof(OobjReloc));
  if (!next) return 0;
  obj->relocs = next;
  r = &obj->relocs[obj->reloc_count];
  memset(r, 0, sizeof(*r));
  r->symbol_name = dup_s(symbol_name);
  if (!r->symbol_name) return 0;
  r->section_index = section_index;
  r->offset = offset;
  r->type = type;
  r->addend = addend;
  obj->reloc_count++;
  return 1;
}

int oobj_find_symbol(const OobjObject *obj, const char *name) {
  size_t i;
  for (i = 0; i < obj->symbol_count; ++i) {
    if (strcmp(obj->symbols[i].name, name) == 0) return (int)i;
  }
  return -1;
}

enum { OOBJ_LINE_CAP = 512u * 1024u };

static size_t count_pipes(const char *s) {
  size_t n = 0;
  for (; *s; ++s) {
    if (*s == '|') ++n;
  }
  return n;
}

/* Split `line` in-place on '|'; writes NULs. Returns number of fields (1..maxf). */
static int split_pipe_fields(char *line, char **fields, int maxf) {
  int n = 0;
  char *r = line;
  while (n < maxf) {
    char *q = strchr(r, '|');
    if (!q) {
      fields[n++] = r;
      break;
    }
    *q = '\0';
    fields[n++] = r;
    r = q + 1;
  }
  return n;
}

static int parse_u32_strict(const char *s, int base, uint32_t *out) {
  char *end = NULL;
  unsigned long v;
  if (!s || !s[0]) return 0;
  errno = 0;
  v = strtoul(s, &end, base);
  if (errno == ERANGE || !end || *end != '\0' || end == s || v > (unsigned long)UINT32_MAX) return 0;
  *out = (uint32_t)v;
  return 1;
}

static int parse_i32_strict(const char *s, int base, int32_t *out) {
  char *end = NULL;
  long v;
  if (!s || !s[0]) return 0;
  errno = 0;
  v = strtol(s, &end, base);
  if (errno == ERANGE || !end || *end != '\0' || end == s || v < (long)INT32_MIN || v > (long)INT32_MAX) return 0;
  *out = (int32_t)v;
  return 1;
}

static int parse_u64_strict(const char *s, int base, uint64_t *out) {
  char *end = NULL;
  unsigned long long v;
  if (!s || !s[0]) return 0;
  errno = 0;
  v = strtoull(s, &end, base);
  if (errno == ERANGE || !end || *end != '\0' || end == s) return 0;
  *out = (uint64_t)v;
  return 1;
}

static int parse_i64_strict(const char *s, int base, int64_t *out) {
  char *end = NULL;
  long long v;
  if (!s || !s[0]) return 0;
  errno = 0;
  v = strtoll(s, &end, base);
  if (errno == ERANGE || !end || *end != '\0' || end == s) return 0;
  *out = (int64_t)v;
  return 1;
}

static int oobj_read_body_v2(FILE *f, char *line, OobjObject *out, char *err, size_t err_len, unsigned base_line) {
  unsigned line_no = base_line - 1u;
  while (fgets(line, OOBJ_LINE_CAP, f)) {
    char *fields[8];
    int nf;
    size_t L;
    size_t pipes;
    line_no++;
    if (line[0] == '\n' || line[0] == '#') continue;
    L = strlen(line);
    while (L > 0 && (line[L - 1] == '\n' || line[L - 1] == '\r')) line[--L] = '\0';
    if (L == 0) continue;
    {
      size_t i;
      for (i = 0; i < L && (line[i] == ' ' || line[i] == '\t'); ++i) {
      }
      if (i == L) continue;
    }
    pipes = count_pipes(line);
    if (pipes > 7u) {
      snprintf(err, err_len, "oobj v2 line %u: too many '|' fields (max 8)", line_no);
      oobj_free(out);
      return 0;
    }
    nf = split_pipe_fields(line, fields, 8);
    if (nf < 1 || fields[0][0] == '\0') {
      snprintf(err, err_len, "oobj v2 line %u: empty record type", line_no);
      oobj_free(out);
      return 0;
    }
    if (strcmp(fields[0], "target") == 0) {
      if (pipes != 1u || nf != 2) {
        snprintf(err, err_len, "oobj v2 line %u: target expects exactly 2 pipe-separated fields", line_no);
        oobj_free(out);
        return 0;
      }
      snprintf(out->target, sizeof(out->target), "%s", fields[1]);
    } else if (strcmp(fields[0], "section") == 0) {
      if (pipes != 4u || nf != 5) {
        snprintf(err, err_len, "oobj v2 line %u: section expects exactly 5 pipe-separated fields", line_no);
        oobj_free(out);
        return 0;
      }
      {
        uint8_t *data = NULL;
        size_t data_len = 0;
        uint32_t align = 0;
        uint32_t flags = 0;
        if (!parse_u32_strict(fields[2], 10, &align)) {
          snprintf(err, err_len, "oobj v2 line %u: bad section align", line_no);
          oobj_free(out);
          return 0;
        }
        if (!parse_u32_strict(fields[3], 10, &flags)) {
          snprintf(err, err_len, "oobj v2 line %u: bad section flags", line_no);
          oobj_free(out);
          return 0;
        }
        if (!parse_hex_bytes(fields[4], &data, &data_len)) {
          snprintf(err, err_len, "oobj v2 line %u: bad section hex", line_no);
          oobj_free(out);
          return 0;
        }
        if (!oobj_append_section(out, fields[1], align, flags, data, data_len)) {
          free(data);
          snprintf(err, err_len, "oobj v2 line %u: oom section append", line_no);
          oobj_free(out);
          return 0;
        }
        free(data);
      }
    } else if (strcmp(fields[0], "symbol") == 0) {
      if (pipes != 4u || nf != 5) {
        snprintf(err, err_len, "oobj v2 line %u: symbol expects exactly 5 pipe-separated fields", line_no);
        oobj_free(out);
        return 0;
      }
      {
        int32_t sec = 0;
        uint64_t value = 0;
        if (!parse_i32_strict(fields[2], 10, &sec)) {
          snprintf(err, err_len, "oobj v2 line %u: bad symbol section index", line_no);
          oobj_free(out);
          return 0;
        }
        if (!parse_u64_strict(fields[3], 0, &value)) {
          snprintf(err, err_len, "oobj v2 line %u: bad symbol value", line_no);
          oobj_free(out);
          return 0;
        }
        if (!oobj_append_symbol(out, fields[1], sec, value, strcmp(fields[4], "global") == 0)) {
          snprintf(err, err_len, "oobj v2 line %u: oom symbol append", line_no);
          oobj_free(out);
          return 0;
        }
      }
    } else if (strcmp(fields[0], "reloc") == 0) {
      if (pipes != 5u || nf != 6) {
        snprintf(err, err_len, "oobj v2 line %u: reloc expects exactly 6 pipe-separated fields", line_no);
        oobj_free(out);
        return 0;
      }
      {
        uint32_t sec = 0;
        uint64_t off = 0;
        int64_t addend = 0;
        OobjRelocType rt;
        if (!parse_u32_strict(fields[1], 10, &sec)) {
          snprintf(err, err_len, "oobj v2 line %u: bad reloc section index", line_no);
          oobj_free(out);
          return 0;
        }
        if (!parse_u64_strict(fields[2], 10, &off)) {
          snprintf(err, err_len, "oobj v2 line %u: bad reloc offset", line_no);
          oobj_free(out);
          return 0;
        }
        if (!parse_i64_strict(fields[5], 10, &addend)) {
          snprintf(err, err_len, "oobj v2 line %u: bad reloc addend", line_no);
          oobj_free(out);
          return 0;
        }
        if (strcmp(fields[4], "abs64") == 0) rt = OOBJ_RELOC_ABS64;
        else if (strcmp(fields[4], "pc32") == 0) rt = OOBJ_RELOC_PC32;
        else if (strcmp(fields[4], "pc64") == 0) rt = OOBJ_RELOC_PC64;
        else {
          snprintf(err, err_len, "oobj v2 line %u: unknown reloc type \"%s\"", line_no, fields[4]);
          oobj_free(out);
          return 0;
        }
        if (!oobj_append_reloc(out, sec, off, fields[3], rt, addend)) {
          snprintf(err, err_len, "oobj v2 line %u: oom reloc append", line_no);
          oobj_free(out);
          return 0;
        }
      }
    } else {
      snprintf(err, err_len, "oobj v2 line %u: unknown record type \"%s\"", line_no, fields[0]);
      oobj_free(out);
      return 0;
    }
  }
  return 1;
}

int oobj_read_file(const char *path, OobjObject *out, char *err, size_t err_len) {
  FILE *f = fopen(path, "rb");
  char *line = (char *)malloc(OOBJ_LINE_CAP);
  if (!f) {
    free(line);
    snprintf(err, err_len, "cannot open oobj: %s", path);
    return 0;
  }
  if (!line) {
    fclose(f);
    snprintf(err, err_len, "oom oobj line buffer");
    return 0;
  }
  oobj_init(out);
  if (!fgets(line, OOBJ_LINE_CAP, f)) {
    free(line);
    fclose(f);
    snprintf(err, err_len, "empty oobj: %s", path);
    return 0;
  }
  if (strncmp(line, "OOBJv2", 6) != 0) {
    free(line);
    fclose(f);
    snprintf(err, err_len, "bad oobj magic (expected OOBJv2): %s", path);
    return 0;
  }
  if (!oobj_read_body_v2(f, line, out, err, err_len, 2u)) {
    free(line);
    fclose(f);
    return 0;
  }
  free(line);
  fclose(f);
  return 1;
}

int oobj_write_file(const char *path, const OobjObject *obj, char *err, size_t err_len) {
  FILE *f = fopen(path, "wb");
  size_t i;
  if (!f) {
    snprintf(err, err_len, "cannot write oobj: %s", path);
    return 0;
  }
  fprintf(f, "OOBJv2\n");
  fprintf(f, "target|%s\n", obj->target[0] ? obj->target : "x86_64-unknown-linux-gnu");
  for (i = 0; i < obj->section_count; ++i) {
    char *hex = bytes_to_hex(obj->sections[i].data, obj->sections[i].data_len);
    if (!hex) {
      fclose(f);
      snprintf(err, err_len, "oom section hex");
      return 0;
    }
    fprintf(f, "section|%s|%u|%u|%s\n", obj->sections[i].name, obj->sections[i].align, obj->sections[i].flags, hex);
    free(hex);
  }
  for (i = 0; i < obj->symbol_count; ++i) {
    fprintf(f, "symbol|%s|%d|%llu|%s\n", obj->symbols[i].name, obj->symbols[i].section_index,
            (unsigned long long)obj->symbols[i].value, obj->symbols[i].is_global ? "global" : "local");
  }
  for (i = 0; i < obj->reloc_count; ++i) {
    const char *type =
        obj->relocs[i].type == OOBJ_RELOC_ABS64 ? "abs64" : (obj->relocs[i].type == OOBJ_RELOC_PC64 ? "pc64" : "pc32");
    fprintf(f, "reloc|%u|%llu|%s|%s|%lld\n", obj->relocs[i].section_index, (unsigned long long)obj->relocs[i].offset,
            obj->relocs[i].symbol_name, type, (long long)obj->relocs[i].addend);
  }
  fclose(f);
  return 1;
}
