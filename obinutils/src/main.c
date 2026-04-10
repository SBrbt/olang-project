#include "../../common/oobj.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define OBINUTILS_ERR_SZ 512

static const char *reloc_name(OobjRelocType t) {
  switch (t) {
    case OOBJ_RELOC_PC64:
      return "pc64";
    case OOBJ_RELOC_ABS64:
      return "abs64";
    case OOBJ_RELOC_PC32:
      return "pc32";
    default:
      return "?";
  }
}

static void warn_extra_args(int argc, int last_used) {
  if (argc > last_used) {
    fprintf(stderr, "obinutils: warning: extra arguments ignored\n");
  }
}

static int cmd_read(const char *path) {
  OobjObject o;
  char err[OBINUTILS_ERR_SZ] = {0};
  oobj_init(&o);
  if (!oobj_read_file(path, &o, err, sizeof(err))) {
    fprintf(stderr, "obinutils: %s\n", err);
    return 2;
  }
  printf("target %s\n", o.target);
  printf("sections %zu\n", o.section_count);
  printf("symbols %zu\n", o.symbol_count);
  printf("relocs %zu\n", o.reloc_count);
  oobj_free(&o);
  return 0;
}

static int cmd_nm(const char *path) {
  OobjObject o;
  char err[OBINUTILS_ERR_SZ] = {0};
  size_t i;
  oobj_init(&o);
  if (!oobj_read_file(path, &o, err, sizeof(err))) {
    fprintf(stderr, "obinutils: %s\n", err);
    return 2;
  }
  for (i = 0; i < o.symbol_count; ++i) {
    const char *bind = o.symbols[i].is_global ? "global" : "local";
    if (o.symbols[i].section_index < 0) {
      printf("%s UND 0 %s\n", bind, o.symbols[i].name);
    } else {
      printf("%s %" PRId32 " %" PRIu64 " %s\n", bind, o.symbols[i].section_index,
             (uint64_t)o.symbols[i].value, o.symbols[i].name);
    }
  }
  oobj_free(&o);
  return 0;
}

static int cmd_reloc(const char *path) {
  OobjObject o;
  char err[OBINUTILS_ERR_SZ] = {0};
  size_t i;
  oobj_init(&o);
  if (!oobj_read_file(path, &o, err, sizeof(err))) {
    fprintf(stderr, "obinutils: %s\n", err);
    return 2;
  }
  for (i = 0; i < o.reloc_count; ++i) {
    printf("%" PRIu32 " %" PRIu64 " %s %s %" PRId64 "\n", o.relocs[i].section_index,
           (uint64_t)o.relocs[i].offset, o.relocs[i].symbol_name, reloc_name(o.relocs[i].type),
           (int64_t)o.relocs[i].addend);
  }
  oobj_free(&o);
  return 0;
}

static void diff_report_section(size_t i, const OobjSection *a, const OobjSection *b) {
  const char *an = a->name ? a->name : "?";
  const char *bn = b->name ? b->name : "?";
  if (strcmp(an, bn) != 0) printf("diff section[%zu] name: %s vs %s\n", i, an, bn);
  if (a->align != b->align) printf("diff section[%zu] (%s) align: %" PRIu32 " vs %" PRIu32 "\n", i, an, a->align, b->align);
  if (a->flags != b->flags) printf("diff section[%zu] (%s) flags: %" PRIu32 " vs %" PRIu32 "\n", i, an, a->flags, b->flags);
  if (a->data_len != b->data_len) {
    printf("diff section[%zu] (%s) data_len: %zu vs %zu\n", i, an, a->data_len, b->data_len);
  } else if (a->data_len > 0) {
    if (!a->data || !b->data) {
      if (a->data != b->data) printf("diff section[%zu] (%s) data: pointer mismatch\n", i, an);
    } else if (memcmp(a->data, b->data, a->data_len) != 0) {
      printf("diff section[%zu] (%s) data: content differs (%zu bytes)\n", i, an, a->data_len);
    }
  }
}

static void diff_report_symbol(size_t i, const OobjSymbol *a, const OobjSymbol *b) {
  const char *an = a->name ? a->name : "?";
  const char *bn = b->name ? b->name : "?";
  if (strcmp(an, bn) != 0) printf("diff symbol[%zu] name: %s vs %s\n", i, an, bn);
  if (a->section_index != b->section_index) {
    printf("diff symbol[%zu] (%s) section_index: %" PRId32 " vs %" PRId32 "\n", i, an, a->section_index,
           b->section_index);
  }
  if (a->value != b->value) {
    printf("diff symbol[%zu] (%s) value: %" PRIu64 " vs %" PRIu64 "\n", i, an, (uint64_t)a->value,
           (uint64_t)b->value);
  }
  if (a->is_global != b->is_global) {
    printf("diff symbol[%zu] (%s) is_global: %u vs %u\n", i, an, (unsigned)a->is_global,
           (unsigned)b->is_global);
  }
}

static void diff_report_reloc(size_t i, const OobjReloc *a, const OobjReloc *b) {
  const char *as = a->symbol_name ? a->symbol_name : "?";
  const char *bs = b->symbol_name ? b->symbol_name : "?";
  if (a->section_index != b->section_index) {
    printf("diff reloc[%zu] section_index: %" PRIu32 " vs %" PRIu32 "\n", i, a->section_index, b->section_index);
  }
  if (a->offset != b->offset) {
    printf("diff reloc[%zu] offset: %" PRIu64 " vs %" PRIu64 "\n", i, (uint64_t)a->offset, (uint64_t)b->offset);
  }
  if (strcmp(as, bs) != 0) printf("diff reloc[%zu] symbol: %s vs %s\n", i, as, bs);
  if (a->type != b->type) printf("diff reloc[%zu] (%s) type: %s vs %s\n", i, as, reloc_name(a->type), reloc_name(b->type));
  if (a->addend != b->addend) {
    printf("diff reloc[%zu] (%s) addend: %" PRId64 " vs %" PRId64 "\n", i, as, (int64_t)a->addend,
           (int64_t)b->addend);
  }
}

static int cmd_diff(const char *a, const char *b) {
  OobjObject oa, ob;
  char err[OBINUTILS_ERR_SZ] = {0};
  int rc = 0;
  size_t i;
  size_t nsec, nsym, nrel;
  oobj_init(&oa);
  oobj_init(&ob);
  if (!oobj_read_file(a, &oa, err, sizeof(err))) {
    fprintf(stderr, "obinutils: %s\n", err);
    return 2;
  }
  if (!oobj_read_file(b, &ob, err, sizeof(err))) {
    oobj_free(&oa);
    fprintf(stderr, "obinutils: %s\n", err);
    return 2;
  }
  if (strcmp(oa.target, ob.target) != 0) {
    printf("diff target: %s vs %s\n", oa.target, ob.target);
    rc = 1;
  }
  if (oa.section_count != ob.section_count) {
    printf("diff section_count: %zu vs %zu\n", oa.section_count, ob.section_count);
    rc = 1;
  }
  if (oa.symbol_count != ob.symbol_count) {
    printf("diff symbol_count: %zu vs %zu\n", oa.symbol_count, ob.symbol_count);
    rc = 1;
  }
  if (oa.reloc_count != ob.reloc_count) {
    printf("diff reloc_count: %zu vs %zu\n", oa.reloc_count, ob.reloc_count);
    rc = 1;
  }
  nsec = oa.section_count < ob.section_count ? oa.section_count : ob.section_count;
  for (i = 0; i < nsec; ++i) {
    const OobjSection *sa = &oa.sections[i];
    const OobjSection *sb = &ob.sections[i];
    const char *na = sa->name ? sa->name : "";
    const char *nb = sb->name ? sb->name : "";
    if (strcmp(na, nb) != 0 || sa->align != sb->align || sa->flags != sb->flags || sa->data_len != sb->data_len ||
        (sa->data_len > 0 &&
         ((!sa->data || !sb->data) || memcmp(sa->data, sb->data, sa->data_len) != 0))) {
      diff_report_section(i, sa, sb);
      rc = 1;
    }
  }
  nsym = oa.symbol_count < ob.symbol_count ? oa.symbol_count : ob.symbol_count;
  for (i = 0; i < nsym; ++i) {
    const OobjSymbol *sa = &oa.symbols[i];
    const OobjSymbol *sb = &ob.symbols[i];
    const char *na = sa->name ? sa->name : "";
    const char *nb = sb->name ? sb->name : "";
    if (strcmp(na, nb) != 0 || sa->section_index != sb->section_index || sa->value != sb->value ||
        sa->is_global != sb->is_global) {
      diff_report_symbol(i, sa, sb);
      rc = 1;
    }
  }
  nrel = oa.reloc_count < ob.reloc_count ? oa.reloc_count : ob.reloc_count;
  for (i = 0; i < nrel; ++i) {
    const OobjReloc *ra = &oa.relocs[i];
    const OobjReloc *rb = &ob.relocs[i];
    const char *na = ra->symbol_name ? ra->symbol_name : "";
    const char *nb = rb->symbol_name ? rb->symbol_name : "";
    if (ra->section_index != rb->section_index || ra->offset != rb->offset || strcmp(na, nb) != 0 ||
        ra->type != rb->type || ra->addend != rb->addend) {
      diff_report_reloc(i, ra, rb);
      rc = 1;
    }
  }
  oobj_free(&oa);
  oobj_free(&ob);
  return rc;
}

static void print_obinutils_usage(FILE *out) {
  fprintf(out, "usage: obinutils <command> <args>\n");
  fprintf(out, "commands:\n");
  fprintf(out, "  oobj-read <file.oobj>        section/symbol counts\n");
  fprintf(out, "  oobj-nm <file.oobj>          symbol list\n");
  fprintf(out, "  oobj-reloc <file.oobj>       relocation records\n");
  fprintf(out, "  oobj-diff <a.oobj> <b.oobj>  compare (order-sensitive by index) sections, symbols, relocs\n");
  fprintf(out, "  -h | --help                  this message\n");
}

int main(int argc, char **argv) {
  if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
    print_obinutils_usage(stdout);
    return 0;
  }
  if (argc < 3) {
    print_obinutils_usage(stderr);
    return 2;
  }
  if (strcmp(argv[1], "oobj-read") == 0) {
    warn_extra_args(argc, 3);
    return cmd_read(argv[2]);
  }
  if (strcmp(argv[1], "oobj-nm") == 0) {
    warn_extra_args(argc, 3);
    return cmd_nm(argv[2]);
  }
  if (strcmp(argv[1], "oobj-reloc") == 0) {
    warn_extra_args(argc, 3);
    return cmd_reloc(argv[2]);
  }
  if (strcmp(argv[1], "oobj-diff") == 0) {
    if (argc < 4) {
      fprintf(stderr, "usage: obinutils oobj-diff <a.oobj> <b.oobj>\n");
      return 2;
    }
    warn_extra_args(argc, 4);
    return cmd_diff(argv[2], argv[3]);
  }
  fprintf(stderr, "obinutils: unknown command %s\n", argv[1]);
  return 2;
}
