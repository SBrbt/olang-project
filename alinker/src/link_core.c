#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "link_core.h"

#define LINK_DEFAULT_PAGE_ALIGN 0x1000ull

typedef struct MergedSectionInfo {
  uint64_t *sec_base;
  uint64_t *sec_abs_vaddr;
  size_t sec_count;
} MergedSectionInfo;

static void merged_layout_free(MergedSectionInfo *l) {
  free(l->sec_base);
  free(l->sec_abs_vaddr);
  l->sec_base = NULL;
  l->sec_abs_vaddr = NULL;
  l->sec_count = 0;
}

static void wr_u32le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
  p[2] = (uint8_t)((v >> 16) & 0xff);
  p[3] = (uint8_t)((v >> 24) & 0xff);
}

static void wr_u64le(uint8_t *p, uint64_t v) {
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
  p[2] = (uint8_t)((v >> 16) & 0xff);
  p[3] = (uint8_t)((v >> 24) & 0xff);
  p[4] = (uint8_t)((v >> 32) & 0xff);
  p[5] = (uint8_t)((v >> 40) & 0xff);
  p[6] = (uint8_t)((v >> 48) & 0xff);
  p[7] = (uint8_t)((v >> 56) & 0xff);
}

static int find_defined_symbol(const OobjObject *obj, const char *name) {
  size_t i;
  for (i = 0; i < obj->symbol_count; ++i) {
    if (obj->symbols[i].section_index >= 0 && strcmp(obj->symbols[i].name, name) == 0) return (int)i;
  }
  return -1;
}

static int find_section_by_name(const OobjObject *obj, const char *name) {
  size_t i;
  for (i = 0; i < obj->section_count; ++i) {
    if (strcmp(obj->sections[i].name, name) == 0) return (int)i;
  }
  return -1;
}

static uint64_t align_up64(uint64_t x, uint64_t a) {
  uint64_t m;
  if (a == 0) return x;
  if (x > UINT64_MAX - (a - 1ull)) return x;
  m = (x + a - 1ull) / a * a;
  return m;
}

static int ensure_section(OobjObject *obj, const OobjSection *tpl, int *sec_idx_out, char *err, size_t err_len) {
  int si = find_section_by_name(obj, tpl->name);
  if (si >= 0) {
    if (obj->sections[si].align < tpl->align) obj->sections[si].align = tpl->align;
    obj->sections[si].flags |= tpl->flags;
    *sec_idx_out = si;
    return 1;
  }
  if (!oobj_append_section(obj, tpl->name, tpl->align, tpl->flags, NULL, 0)) {
    snprintf(err, err_len, "merge: oom append section");
    return 0;
  }
  *sec_idx_out = (int)(obj->section_count - 1);
  return 1;
}

static int append_to_section(OobjObject *obj, int sec_idx, const uint8_t *src, size_t src_len, uint64_t *base_off, char *err, size_t err_len) {
  OobjSection *s;
  uint8_t *next;
  if (sec_idx < 0 || (size_t)sec_idx >= obj->section_count) {
    snprintf(err, err_len, "merge: section index out of range");
    return 0;
  }
  s = &obj->sections[sec_idx];
  *base_off = (uint64_t)s->data_len;
  next = (uint8_t *)realloc(s->data, s->data_len + src_len);
  if (!next) {
    snprintf(err, err_len, "merge: oom section grow");
    return 0;
  }
  s->data = next;
  memcpy(s->data + s->data_len, src, src_len);
  s->data_len += src_len;
  return 1;
}

static int resolve_symbol_addr(const OobjObject *merged, const MergedSectionInfo *layout, const char *name, uint64_t *out_addr) {
  int si = find_defined_symbol(merged, name);
  int32_t sec;
  if (si < 0) return 0;
  sec = merged->symbols[si].section_index;
  if (sec < 0 || (size_t)sec >= layout->sec_count) return 0;
  *out_addr = layout->sec_abs_vaddr[(size_t)sec] + merged->symbols[si].value;
  return 1;
}

static int apply_relocations(OobjObject *merged, const MergedSectionInfo *layout, char *err, size_t err_len) {
  size_t i;
  for (i = 0; i < merged->reloc_count; ++i) {
    OobjReloc *r = &merged->relocs[i];
    OobjSection *sec;
    uint64_t S = 0;
    uint64_t P = 0;
    if (r->section_index >= merged->section_count || r->section_index >= layout->sec_count) {
      snprintf(err, err_len, "reloc: section out of range");
      return 0;
    }
    sec = &merged->sections[r->section_index];
    if (!resolve_symbol_addr(merged, layout, r->symbol_name, &S)) {
      snprintf(err, err_len, "reloc: undefined symbol: %s", r->symbol_name);
      return 0;
    }
    if (r->offset >= sec->data_len) {
      snprintf(err, err_len, "reloc: offset out of range");
      return 0;
    }
    P = layout->sec_abs_vaddr[r->section_index] + r->offset;
    if (r->type == OOBJ_RELOC_ABS64) {
      if (r->offset + 8 > sec->data_len) {
        snprintf(err, err_len, "reloc: abs64 overflow");
        return 0;
      }
      wr_u64le(sec->data + r->offset, (uint64_t)((int64_t)S + r->addend));
    } else if (r->type == OOBJ_RELOC_PC32) {
      int64_t disp;
      if (r->offset + 4 > sec->data_len) {
        snprintf(err, err_len, "reloc: pc32 overflow");
        return 0;
      }
      disp = (int64_t)S + r->addend - (int64_t)(P + 4);
      if (disp < (int64_t)INT32_MIN || disp > (int64_t)INT32_MAX) {
        snprintf(err, err_len, "reloc: pc32 out of range for symbol %s", r->symbol_name);
        return 0;
      }
      wr_u32le(sec->data + r->offset, (uint32_t)(int32_t)disp);
    } else if (r->type == OOBJ_RELOC_PC64) {
      int64_t v64;
      if (r->offset + 8 > sec->data_len) {
        snprintf(err, err_len, "reloc: pc64 overflow");
        return 0;
      }
      v64 = (int64_t)S + r->addend - (int64_t)P;
      wr_u64le(sec->data + r->offset, (uint64_t)v64);
    } else {
      snprintf(err, err_len, "reloc: unsupported type");
      return 0;
    }
  }
  return 1;
}

static int merge_objects(const LinkScript *script, OobjObject *merged, char *err, size_t err_len) {
  size_t i, j;
  oobj_init(merged);
  for (i = 0; i < script->input_count; ++i) {
    OobjObject in;
    int *sec_map_idx = NULL;
    uint64_t *sec_map_base = NULL;
    oobj_init(&in);
    if (!oobj_read_file(script->inputs[i], &in, err, err_len)) {
      oobj_free(merged);
      oobj_free(&in);
      return 0;
    }
    sec_map_idx = (int *)malloc(in.section_count * sizeof(int));
    sec_map_base = (uint64_t *)malloc(in.section_count * sizeof(uint64_t));
    if ((!sec_map_idx || !sec_map_base) && in.section_count > 0) {
      free(sec_map_idx);
      free(sec_map_base);
      snprintf(err, err_len, "merge: oom merge maps");
      oobj_free(merged);
      oobj_free(&in);
      return 0;
    }
    for (j = 0; j < in.section_count; ++j) {
      int dst = -1;
      if (!ensure_section(merged, &in.sections[j], &dst, err, err_len)) {
        free(sec_map_idx);
        free(sec_map_base);
        oobj_free(merged);
        oobj_free(&in);
        return 0;
      }
      if (!append_to_section(merged, dst, in.sections[j].data, in.sections[j].data_len, &sec_map_base[j], err, err_len)) {
        free(sec_map_idx);
        free(sec_map_base);
        oobj_free(merged);
        oobj_free(&in);
        return 0;
      }
      sec_map_idx[j] = dst;
    }
    for (j = 0; j < in.symbol_count; ++j) {
      int32_t sec = in.symbols[j].section_index;
      uint64_t val = in.symbols[j].value;
      int prev_def;
      if (sec >= 0) {
        if ((size_t)sec >= in.section_count) {
          free(sec_map_idx);
          free(sec_map_base);
          snprintf(err, err_len, "merge: input symbol section out of range");
          oobj_free(merged);
          oobj_free(&in);
          return 0;
        }
        sec = sec_map_idx[(size_t)sec];
        val += sec_map_base[(size_t)in.symbols[j].section_index];
      }
      prev_def = find_defined_symbol(merged, in.symbols[j].name);
      if (sec >= 0 && prev_def >= 0) {
        if (merged->symbols[prev_def].section_index == sec && merged->symbols[prev_def].value == val) {
          continue;
        }
        free(sec_map_idx);
        free(sec_map_base);
        snprintf(err, err_len, "merge: duplicate defined symbol: %s", in.symbols[j].name);
        oobj_free(merged);
        oobj_free(&in);
        return 0;
      }
      if (!oobj_append_symbol(merged, in.symbols[j].name, sec, val, in.symbols[j].is_global)) {
        free(sec_map_idx);
        free(sec_map_base);
        snprintf(err, err_len, "merge: oom merging symbols");
        oobj_free(merged);
        oobj_free(&in);
        return 0;
      }
    }
    for (j = 0; j < in.reloc_count; ++j) {
      uint32_t dst_sec;
      uint64_t dst_off;
      if (in.relocs[j].section_index >= in.section_count) {
        free(sec_map_idx);
        free(sec_map_base);
        snprintf(err, err_len, "merge: input reloc section out of range");
        oobj_free(merged);
        oobj_free(&in);
        return 0;
      }
      dst_sec = (uint32_t)sec_map_idx[in.relocs[j].section_index];
      dst_off = sec_map_base[in.relocs[j].section_index] + in.relocs[j].offset;
      if (!oobj_append_reloc(merged, dst_sec, dst_off, in.relocs[j].symbol_name, in.relocs[j].type, in.relocs[j].addend)) {
        free(sec_map_idx);
        free(sec_map_base);
        snprintf(err, err_len, "merge: oom merging relocs");
        oobj_free(merged);
        oobj_free(&in);
        return 0;
      }
    }
    free(sec_map_idx);
    free(sec_map_base);
    oobj_free(&in);
  }
  return 1;
}

static uint64_t group_align_after_value(const LinkScript *script, size_t group_index) {
  uint64_t a = script->load_groups[group_index].align_after;
  if (a != 0) return a;
  a = script->load_page_align;
  if (a != 0) return a;
  return LINK_DEFAULT_PAGE_ALIGN;
}

static int effective_group_segment_index(const LinkScript *script, size_t gi) {
  int s = script->load_groups[gi].segment_index;
  if (s < 0) return (int)gi;
  return s;
}

static int verify_load_groups_cover(const OobjObject *merged, const LinkScript *script, char *err, size_t err_len) {
  size_t *marks = NULL;
  size_t gi, k;
  size_t i;
  if (merged->section_count > 0) {
    marks = (size_t *)calloc(merged->section_count, sizeof(size_t));
    if (!marks) {
      snprintf(err, err_len, "groups: oom verify");
      return 0;
    }
  }
  for (gi = 0; gi < script->load_group_count; ++gi) {
    const LinkLoadGroup *g = &script->load_groups[gi];
    for (k = 0; k < g->section_count; ++k) {
      int si = find_section_by_name(merged, g->section_names[k]);
      if (si < 0) continue; /* allow placeholder names for sections not present in any .oobj (e.g. empty .bss) */
      marks[(size_t)si]++;
    }
  }
  for (i = 0; i < merged->section_count; ++i) {
    if (marks[i] != 1u) {
      if (marks) free(marks);
      snprintf(err, err_len, "groups: section %s must appear exactly once in load_groups", merged->sections[i].name);
      return 0;
    }
  }
  if (marks) free(marks);
  return 1;
}

static int ensure_dummy_last_group_if_needed(OobjObject *merged, const LinkScript *script, char *err, size_t err_len) {
  size_t last = script->load_group_count - 1;
  size_t k;
  uint64_t total = 0;
  int sid;
  uint64_t ign;
  const char *dummy_name;
  const LinkLoadGroup *g;
  if (script->segment_count < 2) return 1;
  g = &script->load_groups[last];
  for (k = 0; k < g->section_count; ++k) {
    sid = find_section_by_name(merged, g->section_names[k]);
    if (sid >= 0) total += merged->sections[(size_t)sid].data_len;
  }
  if (total > 0) return 1;
  dummy_name = g->section_names[0];
  {
    static const uint8_t z = 0;
    sid = find_section_by_name(merged, dummy_name);
    if (sid < 0) {
      if (!oobj_append_section(merged, dummy_name, 8, 6, &z, 1)) {
        snprintf(err, err_len, "groups: oom dummy section in last load group");
        return 0;
      }
    } else if (!append_to_section(merged, sid, &z, 1, &ign, err, err_len)) {
      return 0;
    }
  }
  return 1;
}

/* sec_base[i] = offset from first payload byte after stub. sec_abs_vaddr[i] = absolute VMA of section start.
 * Fills group_span[g] = contiguous file contribution per load chunk (group 0 includes stub_len). */
static int compute_load_groups_layout(const OobjObject *merged, const LinkScript *script, size_t stub_len, uint64_t *sec_base, uint64_t *sec_abs_vaddr,
                                      uint64_t *group_span_out, size_t *group_span_count_out, uint64_t *payload_after_stub_out, char *err, size_t err_len) {
  size_t gi, k;
  uint64_t pay = 0;
  uint64_t tail[LINK_SCRIPT_MAX_SEGMENTS];
  size_t ng = script->load_group_count;
  if (ng < 1u || ng > LINK_SCRIPT_MAX_LOAD_GROUPS) {
    snprintf(err, err_len, "layout: invalid load_group_count");
    return 0;
  }
  memset(sec_base, 0, merged->section_count * sizeof(uint64_t));
  memset(sec_abs_vaddr, 0, merged->section_count * sizeof(uint64_t));
  memset(tail, 0, sizeof(tail));
  if (ng > 0u) {
    int s0 = effective_group_segment_index(script, 0u);
    if (s0 >= 0 && (size_t)s0 < LINK_SCRIPT_MAX_SEGMENTS) tail[(size_t)s0] = stub_len;
  }
  for (gi = 0; gi < ng; ++gi) {
    const LinkLoadGroup *g = &script->load_groups[gi];
    uint64_t mark = pay;
    int seg_idx = effective_group_segment_index(script, gi);
    if (seg_idx < 0 || (size_t)seg_idx >= script->segment_count) {
      snprintf(err, err_len, "layout: invalid segment index for load group %zu", gi);
      return 0;
    }
    for (k = 0; k < g->section_count; ++k) {
      int si = find_section_by_name(merged, g->section_names[k]);
      if (si < 0) continue;
      sec_base[(size_t)si] = pay;
      sec_abs_vaddr[(size_t)si] = script->segments[(size_t)seg_idx].vaddr + tail[(size_t)seg_idx];
      tail[(size_t)seg_idx] += merged->sections[(size_t)si].data_len;
      pay += merged->sections[(size_t)si].data_len;
    }
    {
      uint64_t body = pay - mark;
      uint64_t pad = 0;
      if (gi + 1u < ng) {
        uint64_t al = group_align_after_value(script, gi);
        pad = align_up64(stub_len + pay, al) - (stub_len + pay);
        tail[(size_t)effective_group_segment_index(script, gi)] += pad;
        pay += pad;
      }
      if (gi == 0)
        group_span_out[0] = stub_len + body + pad;
      else
        group_span_out[gi] = body + pad;
    }
  }
  *group_span_count_out = ng;
  *payload_after_stub_out = pay;
  return 1;
}

/* Linux x86-64 phase1: call entry (rel32 at +1) then mov edi,eax; mov eax,60; syscall */
static const uint8_t k_linux_amd64_stub14[14] = {0xE8, 0, 0, 0, 0, 0x89, 0xC7, 0xB8, 0x3C, 0x00, 0x00, 0x00, 0x0F, 0x05};

static int fill_and_patch_entry_stub(uint8_t *stub_buf, size_t stub_len, const LinkScript *script, uint64_t entry_off, char *err, size_t err_len) {
  if (stub_len == 0) return 1;
  if (script->call_stub_len == stub_len) {
    memcpy(stub_buf, script->call_stub_bytes, stub_len);
  } else if (stub_len == sizeof(k_linux_amd64_stub14)) {
    memcpy(stub_buf, k_linux_amd64_stub14, stub_len);
  } else {
    snprintf(err, err_len, "image: stub size %zu is neither custom (%zu) nor default (14)", stub_len, script->call_stub_len);
    return 0;
  }
  if (stub_len >= 5u && stub_buf[0] == 0xE8u) {
    int64_t rel64 = (int64_t)(stub_len + entry_off) - 5;
    if (rel64 < (int64_t)INT32_MIN || rel64 > (int64_t)INT32_MAX) {
      snprintf(err, err_len, "image: stub call rel32 out of range");
      return 0;
    }
    wr_u32le(stub_buf + 1, (uint32_t)(int32_t)rel64);
  }
  return 1;
}

static int make_flat_image_load_groups(const OobjObject *merged, const LinkScript *script, const char *entry, size_t stub_len, uint64_t payload_len,
                                       const MergedSectionInfo *layout, char *err, size_t err_len, uint8_t **raw_out, size_t *raw_len_out) {
  int si = find_defined_symbol(merged, entry);
  uint8_t *stub_heap = NULL;
  uint64_t pos = stub_len;
  uint8_t *buf;
  uint64_t entry_off;
  size_t gi, k;
  size_t ng = script->load_group_count;
  if (stub_len > LINK_SCRIPT_MAX_STUB_BYTES) {
    snprintf(err, err_len, "image: stub too large");
    return 0;
  }
  if (si < 0) {
    snprintf(err, err_len, "image: entry symbol not found: %s", entry);
    return 0;
  }
  if (merged->symbols[si].section_index < 0 || (size_t)merged->symbols[si].section_index >= merged->section_count) {
    snprintf(err, err_len, "image: entry symbol is undefined");
    return 0;
  }
  entry_off = layout->sec_base[(size_t)merged->symbols[si].section_index] + merged->symbols[si].value;
  buf = (uint8_t *)malloc(stub_len + payload_len);
  if (!buf) {
    snprintf(err, err_len, "image: oom");
    return 0;
  }
  if (stub_len > 0) {
    stub_heap = (uint8_t *)malloc(stub_len);
    if (!stub_heap) {
      free(buf);
      snprintf(err, err_len, "image: oom");
      return 0;
    }
    if (!fill_and_patch_entry_stub(stub_heap, stub_len, script, entry_off, err, err_len)) {
      free(stub_heap);
      free(buf);
      return 0;
    }
    memcpy(buf, stub_heap, stub_len);
    free(stub_heap);
  }
  for (gi = 0; gi < ng; ++gi) {
    const LinkLoadGroup *g = &script->load_groups[gi];
    for (k = 0; k < g->section_count; ++k) {
      int sec_i = find_section_by_name(merged, g->section_names[k]);
      if (sec_i < 0) continue;
      memcpy(buf + pos, merged->sections[(size_t)sec_i].data, merged->sections[(size_t)sec_i].data_len);
      pos += merged->sections[(size_t)sec_i].data_len;
    }
    if (gi + 1u < ng) {
      uint64_t al = group_align_after_value(script, gi);
      uint64_t pay_so_far = pos - stub_len;
      uint64_t pad = align_up64(stub_len + pay_so_far, al) - (stub_len + pay_so_far);
      if (pad > 0) memset(buf + pos, 0, (size_t)pad);
      pos += pad;
    }
  }
  if (pos != stub_len + payload_len) {
    free(buf);
    snprintf(err, err_len, "image: internal flat size mismatch");
    return 0;
  }
  *raw_out = buf;
  *raw_len_out = stub_len + payload_len;
  return 1;
}

typedef struct FileWriteRange {
  uint64_t begin;
  uint64_t end;
} FileWriteRange;

typedef struct LayoutEvalContext {
  uint64_t entry_addr;
  uint64_t image_size;
  uint64_t payload_rx_size;
  uint64_t payload_rw_size;
  uint64_t payload_g_size[LINK_SCRIPT_MAX_LOAD_GROUPS];
  size_t payload_g_count;
  size_t segment_count_ctx;
  uint64_t seg_file_off[LINK_SCRIPT_MAX_SEGMENTS];
  uint64_t seg_vaddr[LINK_SCRIPT_MAX_SEGMENTS];
  uint64_t seg_align[LINK_SCRIPT_MAX_SEGMENTS];
  uint64_t seg_flags[LINK_SCRIPT_MAX_SEGMENTS];
  uint64_t seg_filesz[LINK_SCRIPT_MAX_SEGMENTS];
  uint64_t seg_memsz[LINK_SCRIPT_MAX_SEGMENTS];
} LayoutEvalContext;

static int parse_u64_expr_number(const char *expr, uint64_t *out) {
  char *end = NULL;
  unsigned long long v;
  if (!expr || !expr[0]) return 0;
  if (expr[0] == '0' && (expr[1] == 'x' || expr[1] == 'X'))
    v = strtoull(expr + 2, &end, 16);
  else
    v = strtoull(expr, &end, 10);
  if (!end || *end != '\0') return 0;
  *out = (uint64_t)v;
  return 1;
}

static int eval_segment_indexed(const char *expr, const LayoutEvalContext *ctx, uint64_t *out) {
  const char *p;
  char *endp;
  unsigned long idx_ul;
  size_t idx;
  if (strncmp(expr, "segment", 7) != 0) return 0;
  p = expr + 7;
  if (*p < '0' || *p > '9') return 0;
  idx_ul = strtoul(p, &endp, 10);
  if (idx_ul >= LINK_SCRIPT_MAX_SEGMENTS) return 0;
  idx = (size_t)idx_ul;
  if (*endp != '_') return 0;
  p = endp + 1;
  if (idx >= ctx->segment_count_ctx) return 0;
  if (strcmp(p, "file_off") == 0) {
    *out = ctx->seg_file_off[idx];
    return 1;
  }
  if (strcmp(p, "vaddr") == 0) {
    *out = ctx->seg_vaddr[idx];
    return 1;
  }
  if (strcmp(p, "align") == 0) {
    *out = ctx->seg_align[idx];
    return 1;
  }
  if (strcmp(p, "flags") == 0) {
    *out = ctx->seg_flags[idx];
    return 1;
  }
  if (strcmp(p, "filesz") == 0) {
    *out = ctx->seg_filesz[idx];
    return 1;
  }
  if (strcmp(p, "memsz") == 0) {
    *out = ctx->seg_memsz[idx];
    return 1;
  }
  return 0;
}

static int eval_value_expr(const char *expr, const LayoutEvalContext *ctx, uint64_t *out) {
  if (parse_u64_expr_number(expr, out)) return 1;
  if (strcmp(expr, "entry_addr") == 0) {
    *out = ctx->entry_addr;
    return 1;
  }
  if (strcmp(expr, "image_size") == 0) {
    *out = ctx->image_size;
    return 1;
  }
  if (strcmp(expr, "payload_rx_size") == 0) {
    *out = ctx->payload_rx_size;
    return 1;
  }
  if (strcmp(expr, "payload_rw_size") == 0) {
    *out = ctx->payload_rw_size;
    return 1;
  }
  if (strncmp(expr, "payload_g", 9) == 0) {
    const char *p = expr + 9;
    char *endp;
    unsigned long idx_ul;
    size_t idx;
    idx_ul = strtoul(p, &endp, 10);
    if (endp == p || strcmp(endp, "_size") != 0) return 0;
    if (idx_ul >= LINK_SCRIPT_MAX_LOAD_GROUPS) return 0;
    idx = (size_t)idx_ul;
    if (idx >= ctx->payload_g_count) return 0;
    *out = ctx->payload_g_size[idx];
    return 1;
  }
  if (eval_segment_indexed(expr, ctx, out)) return 1;
  return 0;
}

static int register_write_range(FileWriteRange *ranges, size_t *range_count, size_t range_cap, uint64_t begin, size_t len, char *err, size_t err_len) {
  size_t i;
  uint64_t end = begin + (uint64_t)len;
  if (len > 0 && end < begin) {
    snprintf(err, err_len, "emit: layout write range overflow");
    return 0;
  }
  for (i = 0; i < *range_count; ++i) {
    if (!(end <= ranges[i].begin || ranges[i].end <= begin)) {
      snprintf(err, err_len, "emit: layout write range overlaps at file_off 0x%llx", (unsigned long long)begin);
      return 0;
    }
  }
  if (*range_count >= range_cap) {
    snprintf(err, err_len, "emit: too many layout write ranges");
    return 0;
  }
  ranges[*range_count].begin = begin;
  ranges[*range_count].end = end;
  (*range_count)++;
  return 1;
}

static int u64_fits_signed_off_t(uint64_t x) {
  uint64_t bits = (uint64_t)(sizeof(off_t) * 8u);
  uint64_t max_pos = ((uint64_t)1 << (bits - 1u)) - 1u;
  return x <= max_pos;
}

static int write_at(FILE *f, uint64_t file_off, const void *data, size_t len, char *err, size_t err_len) {
  off_t seek_off;
  if (!u64_fits_signed_off_t(file_off)) {
    snprintf(err, err_len, "emit: layout file_off too large for this platform");
    return 0;
  }
  seek_off = (off_t)file_off;
  if (fseeko(f, seek_off, SEEK_SET) != 0) {
    snprintf(err, err_len, "emit: layout write seek failed at 0x%llx", (unsigned long long)file_off);
    return 0;
  }
  if (len > 0 && fwrite(data, 1, len, f) != len) {
    snprintf(err, err_len, "emit: layout write failed at 0x%llx", (unsigned long long)file_off);
    return 0;
  }
  return 1;
}

static int emit_by_layout(const LinkScript *script, const uint8_t *raw, size_t raw_len, const uint64_t *payload_g_spans, size_t payload_g_count,
                          const char *output_path, char *err, size_t err_len) {
  FileWriteRange ranges[LINK_SCRIPT_MAX_LAYOUT_OPS];
  size_t range_count = 0;
  size_t i;
  uint64_t sum_spans = 0;
  int payload_written = 0;
  FILE *f;
  LayoutEvalContext ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.entry_addr = script->entry_vma ? script->entry_vma : script->segments[0].vaddr;
  ctx.image_size = (uint64_t)raw_len;
  ctx.segment_count_ctx = script->segment_count;
  for (i = 0; i < payload_g_count && i < LINK_SCRIPT_MAX_LOAD_GROUPS; ++i) {
    ctx.payload_g_size[i] = payload_g_spans[i];
    sum_spans += payload_g_spans[i];
  }
  ctx.payload_g_count = payload_g_count;
  if (sum_spans != (uint64_t)raw_len) {
    snprintf(err, err_len, "emit: internal layout: sum(payload_gN_size) != image size");
    return 0;
  }
  {
    size_t gi;
    for (gi = 0; gi < payload_g_count; ++gi) {
      int sidx = effective_group_segment_index(script, gi);
      if (sidx < 0 || (size_t)sidx >= script->segment_count) {
        snprintf(err, err_len, "emit: invalid segment for load group %zu", gi);
        return 0;
      }
      ctx.seg_filesz[(size_t)sidx] += payload_g_spans[gi];
    }
  }
  if (script->segment_count >= 2u) {
    ctx.payload_rx_size = ctx.seg_filesz[0];
    ctx.payload_rw_size = ctx.seg_filesz[1];
  } else {
    ctx.payload_rx_size = (uint64_t)raw_len;
    ctx.payload_rw_size = 0;
  }
  for (i = 0; i < script->segment_count && i < LINK_SCRIPT_MAX_SEGMENTS; ++i) {
    ctx.seg_file_off[i] = script->segments[i].file_off;
    ctx.seg_vaddr[i] = script->segments[i].vaddr;
    ctx.seg_align[i] = script->segments[i].align;
    ctx.seg_flags[i] = (uint64_t)script->segments[i].p_flags;
    ctx.seg_memsz[i] = ctx.seg_filesz[i];
  }
  for (i = 0; i < script->layout_op_count; ++i) {
    const LinkLayoutOp *op = &script->layout_ops[i];
    size_t len = 0;
    if (op->kind == LINK_LAYOUT_OP_WRITE_BLOB)
      len = op->blob_len;
    else if (op->kind == LINK_LAYOUT_OP_WRITE_U32_LE)
      len = 4u;
    else if (op->kind == LINK_LAYOUT_OP_WRITE_U64_LE)
      len = 8u;
    else if (op->kind == LINK_LAYOUT_OP_WRITE_PAYLOAD) {
      len = raw_len;
      payload_written = 1;
    } else {
      snprintf(err, err_len, "emit: unknown layout operation kind: %d", (int)op->kind);
      return 0;
    }
    if (!register_write_range(ranges, &range_count, LINK_SCRIPT_MAX_LAYOUT_OPS, op->file_off, len, err, err_len)) return 0;
  }
  if (!payload_written) {
    snprintf(err, err_len, "emit: layout must include one write_payload operation");
    return 0;
  }
  f = fopen(output_path, "wb");
  if (!f) {
    snprintf(err, err_len, "emit: cannot open output");
    return 0;
  }
  for (i = 0; i < script->layout_op_count; ++i) {
    const LinkLayoutOp *op = &script->layout_ops[i];
    uint64_t value = 0;
    uint8_t le[8];
    if (op->kind == LINK_LAYOUT_OP_WRITE_BLOB) {
      if (!write_at(f, op->file_off, op->blob, op->blob_len, err, err_len)) {
        fclose(f);
        return 0;
      }
    } else if (op->kind == LINK_LAYOUT_OP_WRITE_U32_LE) {
      if (!eval_value_expr(op->value_expr, &ctx, &value)) {
        fclose(f);
        snprintf(err, err_len, "emit: unknown value expression: %s", op->value_expr);
        return 0;
      }
      if (value > (uint64_t)UINT32_MAX) {
        fclose(f);
        snprintf(err, err_len, "emit: write_u32_le value 0x%llx exceeds 32 bits for expression: %s", (unsigned long long)value, op->value_expr);
        return 0;
      }
      wr_u32le(le, (uint32_t)value);
      if (!write_at(f, op->file_off, le, 4, err, err_len)) {
        fclose(f);
        return 0;
      }
    } else if (op->kind == LINK_LAYOUT_OP_WRITE_U64_LE) {
      if (!eval_value_expr(op->value_expr, &ctx, &value)) {
        fclose(f);
        snprintf(err, err_len, "emit: unknown value expression: %s", op->value_expr);
        return 0;
      }
      wr_u64le(le, value);
      if (!write_at(f, op->file_off, le, 8, err, err_len)) {
        fclose(f);
        return 0;
      }
    } else if (op->kind == LINK_LAYOUT_OP_WRITE_PAYLOAD) {
      if (!write_at(f, op->file_off, raw, raw_len, err, err_len)) {
        fclose(f);
        return 0;
      }
    } else {
      fclose(f);
      snprintf(err, err_len, "emit: unknown layout operation kind: %d", (int)op->kind);
      return 0;
    }
  }
  fclose(f);
  return 1;
}

int alinker_link_oobj_only(const LinkScript *script, const char *output_path, int verbose, char *err, size_t err_len) {
  OobjObject merged;
  MergedSectionInfo layout;
  uint8_t *raw = NULL;
  size_t raw_len = 0;
  size_t stub_len = 0;
  int ok = 0;
  uint64_t group_spans[LINK_SCRIPT_MAX_LOAD_GROUPS];
  size_t group_span_count = 0;
  uint64_t payload_after_stub = 0;
  int entry_sym;

  oobj_init(&merged);
  memset(&layout, 0, sizeof(layout));
  memset(group_spans, 0, sizeof(group_spans));
  if (script->input_count == 0) {
    snprintf(err, err_len, "link: no input objects");
    return 0;
  }
  if (script->segment_count < 1) {
    snprintf(err, err_len, "link: script has no segments");
    return 0;
  }
  if (!merge_objects(script, &merged, err, err_len)) {
    return 0;
  }
  if (verbose) {
    fprintf(stderr, "alinker: merge ok sections=%zu symbols=%zu relocs=%zu\n", merged.section_count, merged.symbol_count, merged.reloc_count);
  }
  entry_sym = find_defined_symbol(&merged, script->entry);
  if (entry_sym < 0) {
    snprintf(err, err_len, "link: entry symbol not found: %s", script->entry);
    oobj_free(&merged);
    return 0;
  }
  if (script->call_stub_len > 0u)
    stub_len = script->call_stub_len;
  else if (script->prepend_call_stub)
    stub_len = 14u;
  else
    stub_len = 0u;

  if (script->load_group_count >= 2u) {
    if (!ensure_dummy_last_group_if_needed(&merged, script, err, err_len)) {
      oobj_free(&merged);
      return 0;
    }
  }
  if (!verify_load_groups_cover(&merged, script, err, err_len)) {
    oobj_free(&merged);
    return 0;
  }

  layout.sec_count = merged.section_count;
  layout.sec_base = (uint64_t *)malloc(merged.section_count * sizeof(uint64_t));
  layout.sec_abs_vaddr = (uint64_t *)malloc(merged.section_count * sizeof(uint64_t));
  if (!layout.sec_base || !layout.sec_abs_vaddr) {
    snprintf(err, err_len, "link: oom layout sec_base");
    merged_layout_free(&layout);
    oobj_free(&merged);
    return 0;
  }

  if (!compute_load_groups_layout(&merged, script, stub_len, layout.sec_base, layout.sec_abs_vaddr, group_spans, &group_span_count, &payload_after_stub, err,
                                 err_len)) {
    merged_layout_free(&layout);
    oobj_free(&merged);
    return 0;
  }
  if (verbose) {
    size_t gi;
    int esi = merged.symbols[(size_t)entry_sym].section_index;
    uint64_t entry_va = layout.sec_abs_vaddr[(size_t)esi] + merged.symbols[(size_t)entry_sym].value;
    fprintf(stderr, "alinker: stub_len=%zu load_groups=%zu payload_base_seg0_vaddr=0x%llx\n", (size_t)stub_len, (size_t)script->load_group_count,
            (unsigned long long)(script->segments[0].vaddr + (uint64_t)stub_len));
    for (gi = 0; gi < group_span_count; ++gi)
      fprintf(stderr, "alinker: payload_g%zu_size=0x%llx\n", gi, (unsigned long long)group_spans[gi]);
    fprintf(stderr, "alinker: payload_after_stub=0x%llx entry_va=0x%llx section \"%s\" sec_off=0x%llx\n", (unsigned long long)payload_after_stub,
            (unsigned long long)entry_va, merged.sections[(size_t)esi].name,
            (unsigned long long)(layout.sec_base[(size_t)esi] + merged.symbols[(size_t)entry_sym].value));
  }
  if (!apply_relocations(&merged, &layout, err, err_len)) {
    merged_layout_free(&layout);
    oobj_free(&merged);
    return 0;
  }
  if (verbose) fprintf(stderr, "alinker: applied %zu relocations\n", merged.reloc_count);
  if (!make_flat_image_load_groups(&merged, script, script->entry, stub_len, payload_after_stub, &layout, err, err_len, &raw, &raw_len)) {
    merged_layout_free(&layout);
    oobj_free(&merged);
    return 0;
  }
  {
    size_t gi;
    uint64_t chk = 0;
    for (gi = 0; gi < group_span_count; ++gi) chk += group_spans[gi];
    if (chk != (uint64_t)raw_len) {
      free(raw);
      raw = NULL;
      merged_layout_free(&layout);
      oobj_free(&merged);
      snprintf(err, err_len, "link: internal group spans do not match image size");
      return 0;
    }
  }
  if (verbose) fprintf(stderr, "alinker: raw image size=%zu\n", raw_len);

  ok = emit_by_layout(script, raw, raw_len, group_spans, group_span_count, output_path, err, err_len);
  if (verbose && ok) fprintf(stderr, "alinker: wrote %s\n", output_path);
  free(raw);
  merged_layout_free(&layout);
  oobj_free(&merged);
  return ok;
}
