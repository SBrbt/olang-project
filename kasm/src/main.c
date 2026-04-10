#include "kasm_asm.h"
#include "kasm_isa.h"

#include "../../common/oobj.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int file_readable(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

static void print_kasm_usage(FILE *out) {
  fprintf(out, "usage: kasm [--isa <file.json>] --in <file.kasm> -o <out.oobj>\n");
  fprintf(out, "       kasm -h | --help\n");
  fprintf(out, "ISA defaults to kasm/isa/x86_64_linux.json; search order:\n");
  fprintf(out, "  $OLANG_TOOLCHAIN_ROOT/kasm/isa/x86_64_linux.json,\n");
  fprintf(out, "  cwd kasm/isa/..., cwd ../kasm/isa/..., <exe-dir>/../kasm/isa/...\n");
}

static int resolve_default_isa(char *out, size_t out_sz, const char *argv0) {
  static const char *const rel[] = {"kasm/isa/x86_64_linux.json", "../kasm/isa/x86_64_linux.json"};
  const char *root = getenv("OLANG_TOOLCHAIN_ROOT");
  size_t k;
  if (root && root[0]) {
    snprintf(out, out_sz, "%s/kasm/isa/x86_64_linux.json", root);
    if (file_readable(out)) return 1;
  }
  for (k = 0; k < sizeof(rel) / sizeof(rel[0]); ++k) {
    snprintf(out, out_sz, "%s", rel[k]);
    if (file_readable(out)) return 1;
  }
  if (argv0 && argv0[0]) {
    char dir[2048];
    char *slash;
    snprintf(dir, sizeof(dir), "%s", argv0);
    slash = strrchr(dir, '/');
    if (slash) {
      *slash = '\0';
      snprintf(out, out_sz, "%s/../kasm/isa/x86_64_linux.json", dir);
      if (file_readable(out)) return 1;
    }
  }
  snprintf(out, out_sz, "%s", rel[0]);
  return 0;
}

int main(int argc, char **argv) {
  const char *input = NULL;
  const char *output = NULL;
  const char *isa_path = NULL;
  char isa_buf[4096];
  IsaSpec isa;
  OobjObject obj;
  char err[512] = {0};
  int i;
  int cr;

  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_kasm_usage(stdout);
      return 0;
    }
    if (strcmp(argv[i], "--in") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "kasm: missing value for --in\n");
        print_kasm_usage(stderr);
        return 2;
      }
      input = argv[++i];
      continue;
    }
    if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "kasm: missing value for -o\n");
        print_kasm_usage(stderr);
        return 2;
      }
      output = argv[++i];
      continue;
    }
    if (strcmp(argv[i], "--isa") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "kasm: missing value for --isa\n");
        print_kasm_usage(stderr);
        return 2;
      }
      isa_path = argv[++i];
      continue;
    }
    fprintf(stderr, "kasm: unknown argument: %s\n", argv[i]);
    print_kasm_usage(stderr);
    return 2;
  }
  if (!input || !output) {
    print_kasm_usage(stderr);
    return 2;
  }
  if (!isa_path) {
    if (!resolve_default_isa(isa_buf, sizeof(isa_buf), argc > 0 ? argv[0] : NULL)) {
      fprintf(stderr, "kasm: cannot find ISA JSON (try --isa or set OLANG_TOOLCHAIN_ROOT)\n");
      return 2;
    }
    isa_path = isa_buf;
  }

  oobj_init(&obj);
  memset(&isa, 0, sizeof(isa));
  if (!isa_load(isa_path, &isa, err, sizeof(err))) {
    fprintf(stderr, "kasm: %s\n", err);
    return 3;
  }

  cr = kasm_compile(&isa, input, output, &obj);
  isa_free(&isa);
  oobj_free(&obj);
  return cr == 0 ? 0 : cr;
}
