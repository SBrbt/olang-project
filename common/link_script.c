#include "link_script.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void link_script_init(LinkScript *s) {
  memset(s, 0, sizeof(*s));
  /* No layout/format defaults: link_script_load_json requires explicit fields in the script. */
  s->prepend_call_stub = -1;
  s->entry_vma = 0;
}

void link_script_free(LinkScript *s) {
  size_t i;
  for (i = 0; i < s->input_count; ++i) free(s->inputs[i]);
  for (i = 0; i < s->layout_op_count; ++i) free(s->layout_ops[i].blob);
  free(s->inputs);
  memset(s, 0, sizeof(*s));
}

static char *load_file(const char *path, size_t *len_out) {
  FILE *f = fopen(path, "rb");
  char *buf;
  long n;
  if (!f) return NULL;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }
  n = ftell(f);
  if (n < 0) {
    fclose(f);
    return NULL;
  }
  rewind(f);
  buf = (char *)malloc((size_t)n + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  if (n > 0 && fread(buf, 1, (size_t)n, f) != (size_t)n) {
    free(buf);
    fclose(f);
    return NULL;
  }
  fclose(f);
  buf[n] = '\0';
  *len_out = (size_t)n;
  return buf;
}

static const char *skip_ws(const char *p) {
  while (*p && isspace((unsigned char)*p)) p++;
  return p;
}

/* p points at opening ". On success sets content [cstart,cend), *after past closing ". */
static int scan_json_string(const char *p, const char *end, const char **cstart, const char **cend, const char **after) {
  const char *s;
  if (p >= end || *p != '"') return 0;
  s = p + 1;
  *cstart = s;
  while (s < end) {
    if (*s == '\\') {
      if (s + 1 >= end) return 0;
      s += 2;
      continue;
    }
    if (*s == '"') {
      *cend = s;
      *after = s + 1;
      return 1;
    }
    s++;
  }
  return 0;
}

/*
 * Top-level object only [start,end). Only a double-quoted token whose raw content equals `key`
 * and is immediately followed (after optional WS) by `:` counts as a key — not a substring inside
 * another string value.
 */
static const char *find_key_string_bounded(const char *start, const char *end, const char *key) {
  size_t klen = strlen(key);
  const char *p = start;
  while (p < end) {
    if (*p != '"') {
      p++;
      continue;
    }
    {
      const char *cstart, *cend, *after;
      if (!scan_json_string(p, end, &cstart, &cend, &after)) {
        p++;
        continue;
      }
      if ((size_t)(cend - cstart) == klen && memcmp(cstart, key, klen) == 0) {
        const char *q = skip_ws(after);
        if (q < end && *q == ':') return p;
      }
      p = after;
    }
  }
  return NULL;
}

static const char *find_key_colon_bounded(const char *start, const char *end, const char *key) {
  /* Same as find_key_string_bounded: JSON requires ':' after key. */
  return find_key_string_bounded(start, end, key);
}

static int parse_json_string_after(const char *p, char *out, size_t cap, const char *key_ctx, char *err, size_t err_len) {
  const char *q = strchr(p, ':');
  const char *s, *e;
  size_t n;
  size_t max_chars;
  if (!q) return 0;
  q = skip_ws(q + 1);
  if (*q != '"') return 0;
  s = q + 1;
  e = strchr(s, '"');
  if (!e) return 0;
  n = (size_t)(e - s);
  if (cap == 0) return 0;
  max_chars = cap - 1u;
  if (n > max_chars) {
    snprintf(err, err_len, "link script: value for \"%s\" too long (max %zu characters)", key_ctx, max_chars);
    return 0;
  }
  memcpy(out, s, n);
  out[n] = '\0';
  return 1;
}

static uint64_t parse_u64_maybe_hex(const char *s) {
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) return strtoull(s + 2, NULL, 16);
  return strtoull(s, NULL, 10);
}

static int parse_json_bool_after(const char *p, int *out) {
  const char *q = strchr(p, ':');
  if (!q) return 0;
  q = skip_ws(q + 1);
  if (strncmp(q, "true", 4) == 0 && !isalnum((unsigned char)q[4])) {
    *out = 1;
    return 1;
  }
  if (strncmp(q, "false", 5) == 0 && !isalnum((unsigned char)q[5])) {
    *out = 0;
    return 1;
  }
  return 0;
}

static const char *skip_json_string(const char *p) {
  if (*p != '"') return p;
  ++p;
  for (; *p; ++p) {
    if (*p == '\\' && p[1]) {
      ++p;
      continue;
    }
    if (*p == '"') return p;
  }
  return p;
}

static const char *object_end(const char *brace) {
  int depth = 0;
  const char *p;
  if (!brace || *brace != '{') return NULL;
  for (p = brace; *p; ++p) {
    if (*p == '"') {
      p = skip_json_string(p);
      if (!*p) break;
      continue;
    }
    if (*p == '{')
      ++depth;
    else if (*p == '}') {
      --depth;
      if (depth == 0) return p;
    }
  }
  return NULL;
}

static const char *array_end(const char *bracket) {
  int depth = 0;
  const char *p;
  if (!bracket || *bracket != '[') return NULL;
  for (p = bracket; *p; ++p) {
    if (*p == '"') {
      p = skip_json_string(p);
      if (!*p) break;
      continue;
    }
    if (*p == '[')
      ++depth;
    else if (*p == ']') {
      --depth;
      if (depth == 0) return p;
    }
  }
  return NULL;
}

/* [start,end) is a JSON object body (from `{` through `}`); keys are JSON string tokens only.
 * Returns 0 if key absent, 1 if present and copied, -1 if present but parse error (err set). */
static int parse_string_key_in_object(const char *start, const char *end, const char *key, char *out, size_t cap,
                                      char *err, size_t err_len) {
  const char *kp = find_key_string_bounded(start, end, key);
  if (!kp) return 0;
  if (!parse_json_string_after(kp, out, cap, key, err, err_len)) return -1;
  return 1;
}

static uint32_t flags_from_string(const char *s) {
  uint32_t f = 0;
  if (strchr(s, 'r') || strchr(s, 'R')) f |= 4u;
  if (strchr(s, 'w') || strchr(s, 'W')) f |= 2u;
  if (strchr(s, 'x') || strchr(s, 'X')) f |= 1u;
  if (f == 0) f = 5u; /* default rx */
  return f;
}

static int parse_one_segment(const char *brace, LinkSegment *seg, char *err, size_t err_len) {
  const char *end = object_end(brace);
  char tmp[128];
  int r;
  if (!end) {
    snprintf(err, err_len, "invalid segment object");
    return 0;
  }
  memset(seg, 0, sizeof(*seg));
  seg->align = 0x1000ull;
  seg->p_flags = 5u;
  r = parse_string_key_in_object(brace, end, "file_off", tmp, sizeof(tmp), err, err_len);
  if (r < 0) return 0;
  if (r > 0) seg->file_off = parse_u64_maybe_hex(tmp);
  r = parse_string_key_in_object(brace, end, "vaddr", tmp, sizeof(tmp), err, err_len);
  if (r < 0) return 0;
  if (r > 0) seg->vaddr = parse_u64_maybe_hex(tmp);
  r = parse_string_key_in_object(brace, end, "align", tmp, sizeof(tmp), err, err_len);
  if (r < 0) return 0;
  if (r > 0) seg->align = parse_u64_maybe_hex(tmp);
  r = parse_string_key_in_object(brace, end, "flags", tmp, sizeof(tmp), err, err_len);
  if (r < 0) return 0;
  if (r > 0) seg->p_flags = flags_from_string(tmp);
  return 1;
}

static int parse_segments_array(const char *buf_start, const char *buf_end, LinkScript *out, char *err, size_t err_len) {
  const char *p = find_key_string_bounded(buf_start, buf_end, "segments");
  const char *q, *brace;
  if (!p) {
    snprintf(err, err_len, "link script: required key \"segments\" is missing");
    return 0;
  }
  q = strchr(p, '[');
  if (!q) {
    snprintf(err, err_len, "segments: expected [");
    return 0;
  }
  p = q + 1;
  for (;;) {
    p = skip_ws(p);
    if (!*p) {
      snprintf(err, err_len, "segments: unclosed array");
      return 0;
    }
    if (*p == ']') break;
    if (*p != '{') {
      snprintf(err, err_len, "segments: expected '{' or ']'");
      return 0;
    }
    brace = p;
    if (out->segment_count >= LINK_SCRIPT_MAX_SEGMENTS) {
      snprintf(err, err_len, "too many segments (max %d)", LINK_SCRIPT_MAX_SEGMENTS);
      return 0;
    }
    if (!parse_one_segment(brace, &out->segments[out->segment_count], err, err_len)) {
      return 0;
    }
    out->segment_count++;
    p = object_end(brace);
    if (!p) {
      snprintf(err, err_len, "unclosed segment object");
      return 0;
    }
    p = p + 1;
    p = skip_ws(p);
    if (!*p) {
      snprintf(err, err_len, "segments: unclosed array");
      return 0;
    }
    if (*p == ']') break;
    if (*p != ',') {
      snprintf(err, err_len, "segments: expected ',' or ']'");
      return 0;
    }
    p++;
    p = skip_ws(p);
    if (*p == ']') {
      snprintf(err, err_len, "segments: trailing comma");
      return 0;
    }
  }
  return 1;
}

static int parse_hex_blob_n(const char *hex, size_t n, unsigned char **out_buf, size_t *out_len, char *err, size_t err_len) {
  size_t i;
  unsigned char *buf;
  int hi, lo;
  if (!hex || n == 0u) {
    snprintf(err, err_len, "layout.write_blob.hex must not be empty");
    return 0;
  }
  if ((n & 1u) != 0u) {
    snprintf(err, err_len, "layout.write_blob.hex must have even length");
    return 0;
  }
  buf = (unsigned char *)malloc(n / 2u);
  if (!buf) {
    snprintf(err, err_len, "oom parsing layout blob");
    return 0;
  }
  for (i = 0; i < n; i += 2u) {
    char a = hex[i];
    char b = hex[i + 1u];
    if (!isxdigit((unsigned char)a) || !isxdigit((unsigned char)b)) {
      free(buf);
      snprintf(err, err_len, "layout.write_blob.hex has non-hex byte at %zu", i);
      return 0;
    }
    hi = isdigit((unsigned char)a) ? a - '0' : (tolower((unsigned char)a) - 'a' + 10);
    lo = isdigit((unsigned char)b) ? b - '0' : (tolower((unsigned char)b) - 'a' + 10);
    buf[i / 2u] = (unsigned char)((hi << 4) | lo);
  }
  *out_buf = buf;
  *out_len = n / 2u;
  return 1;
}

static int decode_hex_to_fixed_buf(const char *hex, size_t n, unsigned char *out, size_t out_cap, size_t *out_len, const char *key_ctx, char *err,
                                   size_t err_len) {
  size_t i;
  int hi, lo;
  if (n == 0u) {
    *out_len = 0;
    return 1;
  }
  if ((n & 1u) != 0u) {
    snprintf(err, err_len, "link script: \"%s\" hex must have even length", key_ctx);
    return 0;
  }
  if (n / 2u > out_cap) {
    snprintf(err, err_len, "link script: \"%s\" too long (max %zu bytes)", key_ctx, out_cap);
    return 0;
  }
  for (i = 0; i < n; i += 2u) {
    char a = hex[i];
    char b = hex[i + 1u];
    if (!isxdigit((unsigned char)a) || !isxdigit((unsigned char)b)) {
      snprintf(err, err_len, "link script: \"%s\" has non-hex at %zu", key_ctx, i);
      return 0;
    }
    hi = isdigit((unsigned char)a) ? a - '0' : (tolower((unsigned char)a) - 'a' + 10);
    lo = isdigit((unsigned char)b) ? b - '0' : (tolower((unsigned char)b) - 'a' + 10);
    out[i / 2u] = (unsigned char)((hi << 4) | lo);
  }
  *out_len = n / 2u;
  return 1;
}

/*
 * String value for key inside [obj_start, obj_end); hex field has no escapes per spec — scan until closing ".
 */
static int parse_json_string_value_bounded(const char *obj_start, const char *obj_end, const char *key, const char **val_start, size_t *val_len, char *err, size_t err_len) {
  const char *p;
  const char *s;
  const char *t;
  p = find_key_string_bounded(obj_start, obj_end, key);
  if (!p) {
    snprintf(err, err_len, "missing string key \"%s\"", key);
    return 0;
  }
  p = strchr(p, ':');
  if (!p || p >= obj_end) {
    snprintf(err, err_len, "expected ':' after \"%s\"", key);
    return 0;
  }
  p = skip_ws(p + 1);
  if (p >= obj_end || *p != '"') {
    snprintf(err, err_len, "expected string value for \"%s\"", key);
    return 0;
  }
  s = p + 1;
  for (t = s; t < obj_end && *t != '"'; ++t) {
  }
  if (t >= obj_end || *t != '"') {
    snprintf(err, err_len, "unclosed string for \"%s\"", key);
    return 0;
  }
  *val_start = s;
  *val_len = (size_t)(t - s);
  return 1;
}

static int parse_one_layout_op(const char *brace, LinkLayoutOp *op, char *err, size_t err_len) {
  const char *end = object_end(brace);
  char kind[32];
  char tmp[256];
  int r;
  if (!end) {
    snprintf(err, err_len, "layout operation object is not closed");
    return 0;
  }
  memset(op, 0, sizeof(*op));
  r = parse_string_key_in_object(brace, end, "op", kind, sizeof(kind), err, err_len);
  if (r < 0) return 0;
  if (r == 0) {
    snprintf(err, err_len, "layout operation requires string key \"op\"");
    return 0;
  }
  r = parse_string_key_in_object(brace, end, "file_off", tmp, sizeof(tmp), err, err_len);
  if (r < 0) return 0;
  if (r == 0) {
    snprintf(err, err_len, "layout operation %s requires string key \"file_off\"", kind);
    return 0;
  }
  op->file_off = parse_u64_maybe_hex(tmp);
  if (strcmp(kind, "write_blob") == 0) {
    const char *hex_start;
    size_t hex_len;
    if (!parse_json_string_value_bounded(brace, end, "hex", &hex_start, &hex_len, err, err_len)) return 0;
    if (!parse_hex_blob_n(hex_start, hex_len, &op->blob, &op->blob_len, err, err_len)) return 0;
    op->kind = LINK_LAYOUT_OP_WRITE_BLOB;
    return 1;
  }
  if (strcmp(kind, "write_u32_le") == 0 || strcmp(kind, "write_u64_le") == 0) {
    r = parse_string_key_in_object(brace, end, "value", op->value_expr, sizeof(op->value_expr), err, err_len);
    if (r < 0) return 0;
    if (r == 0) {
      snprintf(err, err_len, "layout operation %s requires string key \"value\"", kind);
      return 0;
    }
    op->kind = (strcmp(kind, "write_u32_le") == 0) ? LINK_LAYOUT_OP_WRITE_U32_LE : LINK_LAYOUT_OP_WRITE_U64_LE;
    return 1;
  }
  if (strcmp(kind, "write_payload") == 0) {
    op->kind = LINK_LAYOUT_OP_WRITE_PAYLOAD;
    return 1;
  }
  snprintf(err, err_len, "layout operation has unsupported op: %s", kind);
  return 0;
}

static void link_script_default_two_groups(LinkLoadGroup *g0, LinkLoadGroup *g1) {
  snprintf(g0->section_names[0], LINK_SCRIPT_SECTION_NAME_CAP, ".text");
  snprintf(g0->section_names[1], LINK_SCRIPT_SECTION_NAME_CAP, ".rodata");
  g0->section_count = 2;
  snprintf(g1->section_names[0], LINK_SCRIPT_SECTION_NAME_CAP, ".data");
  snprintf(g1->section_names[1], LINK_SCRIPT_SECTION_NAME_CAP, ".bss");
  g1->section_count = 2;
}

/* Optional "rx_sections": [ "a", "b" ] — same object as top-level JSON. */
static int parse_string_array_key(const char *root_start, const char *root_end, const char *key, char (*out)[LINK_SCRIPT_SECTION_NAME_CAP],
                                  size_t *out_count, char *err, size_t err_len) {
  const char *kp = find_key_string_bounded(root_start, root_end, key);
  const char *r;
  const char *s;
  const char *e;
  size_t n;
  if (!kp) {
    *out_count = 0;
    return 1;
  }
  kp = strchr(kp, ':');
  if (!kp || kp >= root_end) {
    snprintf(err, err_len, "link script: expected ':' after \"%s\"", key);
    return 0;
  }
  r = skip_ws(kp + 1);
  if (r >= root_end || *r != '[') {
    snprintf(err, err_len, "link script: \"%s\" must be a JSON array", key);
    return 0;
  }
  r++;
  *out_count = 0;
  for (;;) {
    r = skip_ws(r);
    if (r >= root_end) {
      snprintf(err, err_len, "link script: unclosed array for \"%s\"", key);
      return 0;
    }
    if (*r == ']') {
      break;
    }
    if (*r != '"') {
      snprintf(err, err_len, "link script: \"%s\" array must contain only strings", key);
      return 0;
    }
    s = r + 1;
    e = strchr(s, '"');
    if (!e || e >= root_end) {
      snprintf(err, err_len, "link script: bad string in \"%s\"", key);
      return 0;
    }
    n = (size_t)(e - s);
    if (n >= LINK_SCRIPT_SECTION_NAME_CAP) {
      snprintf(err, err_len, "link script: \"%s\" entry too long (max %d chars)", key, LINK_SCRIPT_SECTION_NAME_CAP - 1);
      return 0;
    }
    if (*out_count >= LINK_SCRIPT_MAX_SECTIONS_PER_GROUP) {
      snprintf(err, err_len, "link script: too many entries in \"%s\" (max %d)", key, LINK_SCRIPT_MAX_SECTIONS_PER_GROUP);
      return 0;
    }
    memcpy(out[*out_count], s, n);
    out[*out_count][n] = '\0';
    (*out_count)++;
    r = skip_ws(e + 1);
    if (r >= root_end) {
      snprintf(err, err_len, "link script: unclosed array for \"%s\"", key);
      return 0;
    }
    if (*r == ']') break;
    if (*r != ',') {
      snprintf(err, err_len, "link script: expected ',' or ']' in \"%s\"", key);
      return 0;
    }
    r++;
  }
  return 1;
}

static int parse_one_load_group_object(const char *brace, LinkLoadGroup *g, char *err, size_t err_len) {
  const char *end = object_end(brace);
  int r;
  char tmp[128];
  if (!end) {
    snprintf(err, err_len, "load_groups: invalid object");
    return 0;
  }
  memset(g, 0, sizeof(*g));
  g->segment_index = -1;
  r = parse_string_array_key(brace, end, "sections", g->section_names, &g->section_count, err, err_len);
  if (r == 0) {
    snprintf(err, err_len, "load_groups: each entry needs \"sections\" array");
    return 0;
  }
  if (g->section_count == 0) {
    snprintf(err, err_len, "load_groups: \"sections\" must be non-empty");
    return 0;
  }
  r = parse_string_key_in_object(brace, end, "align_after", tmp, sizeof(tmp), err, err_len);
  if (r < 0) return 0;
  if (r > 0) g->align_after = parse_u64_maybe_hex(tmp);
  r = parse_string_key_in_object(brace, end, "segment_index", tmp, sizeof(tmp), err, err_len);
  if (r < 0) return 0;
  if (r > 0) {
    uint64_t v = parse_u64_maybe_hex(tmp);
    if (v >= (uint64_t)LINK_SCRIPT_MAX_SEGMENTS) {
      snprintf(err, err_len, "load_groups: segment_index out of range (max %d)", LINK_SCRIPT_MAX_SEGMENTS);
      return 0;
    }
    g->segment_index = (int32_t)v;
  }
  return 1;
}

static int parse_load_groups_array(const char *buf_start, const char *buf_end, LinkScript *out, char *err, size_t err_len) {
  const char *p = find_key_string_bounded(buf_start, buf_end, "load_groups");
  const char *brace;
  if (!p) {
    out->load_group_count = 0;
    return 1;
  }
  p = strchr(p, '[');
  if (!p) {
    snprintf(err, err_len, "load_groups: expected [");
    return 0;
  }
  p++;
  out->load_group_count = 0;
  for (;;) {
    p = skip_ws(p);
    if (!*p) {
      snprintf(err, err_len, "load_groups: unclosed array");
      return 0;
    }
    if (*p == ']') break;
    if (*p != '{') {
      snprintf(err, err_len, "load_groups: expected '{' or ']'");
      return 0;
    }
    brace = p;
    if (out->load_group_count >= LINK_SCRIPT_MAX_LOAD_GROUPS) {
      snprintf(err, err_len, "load_groups: too many groups (max %d)", LINK_SCRIPT_MAX_LOAD_GROUPS);
      return 0;
    }
    if (!parse_one_load_group_object(brace, &out->load_groups[out->load_group_count], err, err_len)) return 0;
    out->load_group_count++;
    p = object_end(brace);
    if (!p) {
      snprintf(err, err_len, "load_groups: unclosed object");
      return 0;
    }
    p = p + 1;
    p = skip_ws(p);
    if (!*p) {
      snprintf(err, err_len, "load_groups: unclosed array");
      return 0;
    }
    if (*p == ']') break;
    if (*p != ',') {
      snprintf(err, err_len, "load_groups: expected ',' or ']'");
      return 0;
    }
    p++;
  }
  return 1;
}

static int finalize_load_groups_legacy(LinkScript *out, const char *root_start, const char *root_end, char *err, size_t err_len) {
  uint64_t align0;
  if (out->load_group_count > 0u) return 1;
  if (!parse_string_array_key(root_start, root_end, "rx_sections", out->load_groups[0].section_names, &out->load_groups[0].section_count, err, err_len))
    return 0;
  if (!parse_string_array_key(root_start, root_end, "rw_sections", out->load_groups[1].section_names, &out->load_groups[1].section_count, err, err_len))
    return 0;
  if (out->load_groups[0].section_count == 0u && out->load_groups[1].section_count == 0u) link_script_default_two_groups(&out->load_groups[0], &out->load_groups[1]);
  else {
    if (out->load_groups[0].section_count == 0u) {
      snprintf(out->load_groups[0].section_names[0], LINK_SCRIPT_SECTION_NAME_CAP, ".text");
      snprintf(out->load_groups[0].section_names[1], LINK_SCRIPT_SECTION_NAME_CAP, ".rodata");
      out->load_groups[0].section_count = 2;
    }
    if (out->load_groups[1].section_count == 0u) {
      snprintf(out->load_groups[1].section_names[0], LINK_SCRIPT_SECTION_NAME_CAP, ".data");
      snprintf(out->load_groups[1].section_names[1], LINK_SCRIPT_SECTION_NAME_CAP, ".bss");
      out->load_groups[1].section_count = 2;
    }
  }
  align0 = out->load_page_align ? out->load_page_align : 0x1000ull;
  out->load_groups[0].align_after = align0;
  out->load_groups[1].align_after = 0;
  out->load_groups[0].segment_index = -1;
  out->load_groups[1].segment_index = -1;
  out->load_group_count = 2;
  return 1;
}

static int parse_layout_array(const char *buf_start, const char *buf_end, LinkScript *out, char *err, size_t err_len) {
  const char *p = find_key_string_bounded(buf_start, buf_end, "layout");
  const char *q;
  const char *brace;
  if (!p) {
    snprintf(err, err_len, "link script: required key \"layout\" is missing");
    return 0;
  }
  q = strchr(p, '[');
  if (!q) {
    snprintf(err, err_len, "layout: expected [");
    return 0;
  }
  if (!array_end(q)) {
    snprintf(err, err_len, "layout: unclosed array");
    return 0;
  }
  p = q + 1;
  for (;;) {
    p = skip_ws(p);
    if (!*p) {
      snprintf(err, err_len, "layout: unclosed array");
      return 0;
    }
    if (*p == ']') break;
    if (*p != '{') {
      snprintf(err, err_len, "layout: expected '{' or ']'");
      return 0;
    }
    brace = p;
    if (out->layout_op_count >= LINK_SCRIPT_MAX_LAYOUT_OPS) {
      snprintf(err, err_len, "too many layout operations (max %d)", LINK_SCRIPT_MAX_LAYOUT_OPS);
      return 0;
    }
    if (!parse_one_layout_op(brace, &out->layout_ops[out->layout_op_count], err, err_len)) return 0;
    out->layout_op_count++;
    p = object_end(brace);
    if (!p) {
      snprintf(err, err_len, "unclosed layout operation object");
      return 0;
    }
    p = p + 1;
    p = skip_ws(p);
    if (!*p) {
      snprintf(err, err_len, "layout: unclosed array");
      return 0;
    }
    if (*p == ']') break;
    if (*p != ',') {
      snprintf(err, err_len, "layout: expected ',' or ']'");
      return 0;
    }
    p++;
    p = skip_ws(p);
    if (*p == ']') {
      snprintf(err, err_len, "layout: trailing comma");
      return 0;
    }
  }
  return 1;
}

int link_script_load_json_buffer(const char *buf, LinkScript *out, char *err, size_t err_len) {
  const char *root_start;
  const char *root_end;
  const char *p;
  char tmp[128];
  int pb;

  link_script_init(out);
  root_start = skip_ws(buf);
  if (*root_start != '{') {
    snprintf(err, err_len, "link script: expected top-level '{'");
    return 0;
  }
  root_end = object_end(root_start);
  if (!root_end) {
    snprintf(err, err_len, "link script: unclosed top-level object");
    return 0;
  }
  root_end++; /* past '}' */
  if (find_key_colon_bounded(root_start, root_end, "output") || find_key_colon_bounded(root_start, root_end, "elf")) {
    snprintf(err,
             err_len,
             "link script: legacy keys \"output\"/\"elf\" are removed; use top-level \"layout\" operations");
    return 0;
  }
  p = find_key_string_bounded(root_start, root_end, "entry");
  if (!p) {
    snprintf(err, err_len, "link script: required string \"entry\" is missing or empty");
    return 0;
  }
  if (!parse_json_string_after(p, out->entry, sizeof(out->entry), "entry", err, err_len)) {
    return 0;
  }
  if (!out->entry[0]) {
    snprintf(err, err_len, "link script: required string \"entry\" is missing or empty");
    return 0;
  }
  p = find_key_string_bounded(root_start, root_end, "prepend_call_stub");
  if (!p || !parse_json_bool_after(p, &pb)) {
    snprintf(err, err_len, "link script: required boolean \"prepend_call_stub\" is missing");
    return 0;
  }
  out->prepend_call_stub = pb;
  out->call_stub_len = 0;
  p = find_key_string_bounded(root_start, root_end, "call_stub_hex");
  if (p) {
    char hex_tmp[LINK_SCRIPT_MAX_STUB_BYTES * 2u + 8u];
    if (!parse_json_string_after(p, hex_tmp, sizeof(hex_tmp), "call_stub_hex", err, err_len)) return 0;
    if (hex_tmp[0]) {
      if (!decode_hex_to_fixed_buf(hex_tmp, strlen(hex_tmp), out->call_stub_bytes, LINK_SCRIPT_MAX_STUB_BYTES, &out->call_stub_len, "call_stub_hex", err,
                                   err_len))
        return 0;
    }
  }
  p = find_key_string_bounded(root_start, root_end, "entry_vma");
  if (p) {
    if (!parse_json_string_after(p, tmp, sizeof(tmp), "entry_vma", err, err_len)) return 0;
    out->entry_vma = parse_u64_maybe_hex(tmp);
  }
  p = find_key_string_bounded(root_start, root_end, "load_page_align");
  if (p) {
    if (!parse_json_string_after(p, tmp, sizeof(tmp), "load_page_align", err, err_len)) return 0;
    {
      uint64_t a = parse_u64_maybe_hex(tmp);
      if (a != 0) out->load_page_align = a;
    }
  }
  if (!parse_load_groups_array(root_start, root_end, out, err, err_len)) return 0;
  if (!finalize_load_groups_legacy(out, root_start, root_end, err, err_len)) return 0;
  if (out->load_group_count < 1u) {
    snprintf(err, err_len, "link script: \"load_groups\" (or legacy rx_sections/rw_sections) required");
    return 0;
  }
  if (!parse_segments_array(root_start, root_end, out, err, err_len)) return 0;
  if (out->segment_count < 1u) {
    snprintf(err, err_len, "link script: \"segments\" must be a non-empty array");
    return 0;
  }
  {
    size_t gi;
    for (gi = 0; gi < out->load_group_count; ++gi) {
      int s = out->load_groups[gi].segment_index;
      if (s < 0) s = (int)gi;
      if (s < 0 || (size_t)s >= out->segment_count) {
        snprintf(err, err_len, "load_groups: group %zu maps to invalid segment index %d (have %zu segments)", gi, s, out->segment_count);
        return 0;
      }
    }
  }
  if (!parse_layout_array(root_start, root_end, out, err, err_len)) return 0;
  if (out->layout_op_count < 1u) {
    snprintf(err, err_len, "link script: \"layout\" must be a non-empty array");
    return 0;
  }
  return 1;
}

int link_script_load_json(const char *path, LinkScript *out, char *err, size_t err_len) {
  size_t len = 0;
  char *buf = load_file(path, &len);
  int ok;
  (void)len;
  if (!buf) {
    snprintf(err, err_len, "cannot read script: %s", path);
    return 0;
  }
  ok = link_script_load_json_buffer(buf, out, err, err_len);
  free(buf);
  return ok;
}
