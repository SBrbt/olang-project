#include "link_core.h"

#include "../../common/link_script.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *xstrdup(const char *s) {
  size_t n = strlen(s) + 1;
  char *p = (char *)malloc(n);
  if (p) memcpy(p, s, n);
  return p;
}

static void print_alinker_usage(FILE *out) {
  fprintf(out, "usage: alinker [--verbose|-v] --script <link.json> -o <out> -- <input.oobj>...\n");
  fprintf(out, "       link.json fully controls output byte layout\n");
  fprintf(out, "       alinker -h | --help\n");
}

static void free_cli_inputs(LinkScript *s, size_t n_alloc, char **buf) {
  size_t j;
  for (j = 0; j < n_alloc; ++j) free(buf[j]);
  free(buf);
  s->inputs = NULL;
  s->input_count = 0;
}

int main(int argc, char **argv) {
  const char *script_path = NULL;
  const char *output_path = NULL;
  LinkScript script;
  char err[512] = {0};
  char **in_buf = NULL;
  size_t n_in = 0;
  int i;
  int dash = -1;
  int verbose = 0;

  memset(&script, 0, sizeof(script));

  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_alinker_usage(stdout);
      return 0;
    }
    if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      verbose = 1;
      continue;
    }
    if (strcmp(argv[i], "--") == 0) {
      dash = i;
      break;
    }
    if (strcmp(argv[i], "--script") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "alinker: missing value for --script\n");
        print_alinker_usage(stderr);
        return 2;
      }
      script_path = argv[++i];
      continue;
    }
    if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "alinker: missing value for -o\n");
        print_alinker_usage(stderr);
        return 2;
      }
      output_path = argv[++i];
      continue;
    }
    fprintf(stderr, "alinker: unknown argument: %s\n", argv[i]);
    print_alinker_usage(stderr);
    return 2;
  }

  if (!script_path || !output_path || dash < 0) {
    print_alinker_usage(stderr);
    return 2;
  }
  if (dash + 1 >= argc) {
    fprintf(stderr, "alinker: need at least one input .oobj after --\n");
    return 2;
  }

  n_in = (size_t)(argc - dash - 1);
  in_buf = (char **)malloc(n_in * sizeof(char *));
  if (!in_buf) {
    fprintf(stderr, "alinker: oom\n");
    return 2;
  }
  for (i = 0; (size_t)i < n_in; ++i) {
    in_buf[i] = xstrdup(argv[dash + 1 + i]);
    if (!in_buf[i]) {
      free_cli_inputs(&script, (size_t)i, in_buf);
      fprintf(stderr, "alinker: oom\n");
      return 2;
    }
  }
  // moved input file names from argv to in_buf

  if (!link_script_load_json(script_path, &script, err, sizeof(err))) {
    free_cli_inputs(&script, n_in, in_buf);
    link_script_free(&script);
    fprintf(stderr, "alinker: %s\n", err);
    return 3;
  }
  script.inputs = in_buf;
  script.input_count = n_in;
  in_buf = NULL;

  if (!alinker_link_oobj_only(&script, output_path, verbose, err, sizeof(err))) {
    link_script_free(&script);
    fprintf(stderr, "alinker: %s\n", err);
    return 4;
  }
  link_script_free(&script);
  return 0;
}
