#include "frontend/parser.h"
#include "backend/backend.h"
#include "ir.h"

#include "../../common/oobj.h"

#include <stdio.h>
#include <string.h>

static void print_olang_usage(FILE *out) {
  fprintf(out, "usage: olang --target <name> --in <file.ol> -o <out.oobj>\n");
  fprintf(out, "       olang -h | --help\n");
}

int main(int argc, char **argv) {
  const char *input = NULL;
  const char *output = NULL;
  const char *target = NULL;
  const OlTargetInfo *ti;
  const OlBackendVTable *be;
  OlProgram program;
  OobjObject obj;
  char err[512] = {0};
  int i;
  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_olang_usage(stdout);
      return 0;
    }
    if (strcmp(argv[i], "--in") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "olang: missing value for --in\n");
        print_olang_usage(stderr);
        return 2;
      }
      input = argv[++i];
      continue;
    }
    if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "olang: missing value for -o\n");
        print_olang_usage(stderr);
        return 2;
      }
      output = argv[++i];
      continue;
    }
    if (strcmp(argv[i], "--target") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "olang: missing value for --target\n");
        print_olang_usage(stderr);
        return 2;
      }
      target = argv[++i];
      continue;
    }
    fprintf(stderr, "olang: unknown argument: %s\n", argv[i]);
    print_olang_usage(stderr);
    return 2;
  }
  if (!input || !output) {
    print_olang_usage(stderr);
    return 2;
  }
  if (!target) {
    fprintf(stderr, "olang: --target is required\n");
    print_olang_usage(stderr);
    return 2;
  }

  ti = ol_target_info_by_name(target);
  if (!ti) {
    fprintf(stderr, "olang: unknown target %s\n", target);
    return 3;
  }
  be = ol_backend_for_target(ti);
  if (!be) {
    fprintf(stderr, "olang: no backend for target %s (arch %s)\n", target, ti->arch);
    return 3;
  }
  if (!ol_parse_source_file(input, &program, err, sizeof(err))) {
    fprintf(stderr, "olang: %s\n", err);
    return 3;
  }
  if (!be->emit_program(ti, &program, &obj, err, sizeof(err))) {
    ol_program_free(&program);
    fprintf(stderr, "olang: %s\n", err);
    return 4;
  }
  if (!oobj_write_file(output, &obj, err, sizeof(err))) {
    fprintf(stderr, "olang: %s\n", err);
    oobj_free(&obj);
    ol_program_free(&program);
    return 6;
  }
  oobj_free(&obj);
  ol_program_free(&program);
  return 0;
}
