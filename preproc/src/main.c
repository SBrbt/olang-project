/*
 * olprep — minimal line-oriented preprocessor (#include "..." only).
 * Language-agnostic; not tied to OLang syntax.
 */
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

enum { MAX_DEPTH = 64, MAX_STACK = 64 };

static void usage(FILE *out) {
  fprintf(out, "usage: olprep --in <file> -o <out> [-I <dir>]...\n");
}

/* Copy dirname of path into buf (POSIX-like: no trailing slash except root). */
static void path_dirname(const char *path, char *buf, size_t bufsz) {
  size_t len = strlen(path);
  size_t i;
  if (len == 0u) {
    buf[0] = '.';
    buf[1] = '\0';
    return;
  }
  i = len;
  while (i > 0u && path[i - 1u] == '/') i--;
  if (i == 0u) {
    buf[0] = '/';
    buf[1] = '\0';
    return;
  }
  while (i > 0u && path[i - 1u] != '/') i--;
  if (i == 0u) {
    if (path[0] == '/') {
      buf[0] = '/';
      buf[1] = '\0';
    } else {
      buf[0] = '.';
      buf[1] = '\0';
    }
    return;
  }
  if (i >= bufsz) i = bufsz - 1u;
  memcpy(buf, path, i);
  buf[i] = '\0';
}

static int try_realpath_existing(const char *candidate, char resolved[PATH_MAX]) {
  return realpath(candidate, resolved) != NULL;
}

static int resolve_include_path(const char *current_file, const char *inc,
                                char **idirs, size_t n_idirs, char out[PATH_MAX], char *err, size_t err_len) {
  size_t k;
  if (inc[0] == '/') {
    if (try_realpath_existing(inc, out)) return 1;
    snprintf(err, err_len, "include file not found: %s", inc);
    return 0;
  }
  {
    char dir[PATH_MAX];
    char cand[PATH_MAX];
    path_dirname(current_file, dir, sizeof(dir));
    if (snprintf(cand, sizeof(cand), "%s/%s", dir, inc) >= (int)sizeof(cand)) {
      snprintf(err, err_len, "include path too long");
      return 0;
    }
    if (try_realpath_existing(cand, out)) return 1;
  }
  for (k = 0u; k < n_idirs; k++) {
    char cand[PATH_MAX];
    if (snprintf(cand, sizeof(cand), "%s/%s", idirs[k], inc) >= (int)sizeof(cand)) {
      snprintf(err, err_len, "include path too long");
      return 0;
    }
    if (try_realpath_existing(cand, out)) return 1;
  }
  snprintf(err, err_len, "include file not found: \"%s\" (from %s)", inc, current_file);
  return 0;
}

static int stack_contains(const char **stack, int depth, const char *canon) {
  int i;
  for (i = 0; i < depth; i++) {
    if (strcmp(stack[i], canon) == 0) return 1;
  }
  return 0;
}

/* Returns pointer after leading spaces/tabs only (not newline). */
static const char *skip_sptab(const char *s) {
  while (*s == ' ' || *s == '\t') s++;
  return s;
}

/* If line is #include "path", set *path_out to static buffer and return 1. */
static int parse_include_line(const char *line, char path_out[PATH_MAX]) {
  const char *p = skip_sptab(line);
  if (*p != '#') return 0;
  p++;
  p = skip_sptab(p);
  if (strncmp(p, "include", 7) != 0) return 0;
  p += 7;
  if (*p != ' ' && *p != '\t') return 0;
  p = skip_sptab(p);
  if (*p != '"') return 0;
  p++;
  {
    size_t n = 0u;
    while (*p && *p != '"' && *p != '\n' && *p != '\r') {
      if (n + 1u >= PATH_MAX) return 0;
      path_out[n++] = *p++;
    }
    path_out[n] = '\0';
    if (*p != '"') return 0;
    p++;
    p = skip_sptab(p);
    if (*p == '\0' || *p == '\n' || *p == '\r') return 1;
    /* trailing junk on line */
    if (*p == '/' && p[1] == '/') return 1; /* allow // comment */
    return 0;
  }
}

static int process_file(const char *src_path, FILE *out, const char **stack, int depth, char **idirs,
                        size_t n_idirs, char *err, size_t err_len) {
  FILE *in;
  char *line = NULL;
  size_t cap = 0;
  ssize_t nread;
  char canon[PATH_MAX];
  char inc_path[PATH_MAX];

  if (depth >= MAX_DEPTH) {
    snprintf(err, err_len, "include depth exceeded (max %d): %s", MAX_DEPTH, src_path);
    return 0;
  }
  if (!realpath(src_path, canon)) {
    snprintf(err, err_len, "realpath failed: %s: %s", src_path, strerror(errno));
    return 0;
  }
  if (stack_contains(stack, depth, canon)) {
    snprintf(err, err_len, "circular #include involving %s", canon);
    return 0;
  }

  in = fopen(src_path, "rb");
  if (!in) {
    snprintf(err, err_len, "cannot open %s: %s", src_path, strerror(errno));
    return 0;
  }

  stack[depth] = canon;
  depth++;

  while ((nread = getline(&line, &cap, in)) != -1) {
    /* strip trailing \n \r for parse; we re-emit line including newline from getline */
    if (nread > 0 && line[nread - 1] == '\n') {
      line[nread - 1] = '\0';
      nread--;
    }
    if (nread > 0 && line[nread - 1] == '\r') {
      line[nread - 1] = '\0';
      nread--;
    }

    /* detect // at end for include line only */
    if (parse_include_line(line, inc_path)) {
      char resolved[PATH_MAX];
      if (!resolve_include_path(src_path, inc_path, idirs, n_idirs, resolved, err, err_len)) {
        free(line);
        fclose(in);
        return 0;
      }
      if (!process_file(resolved, out, stack, depth, idirs, n_idirs, err, err_len)) {
        free(line);
        fclose(in);
        return 0;
      }
      continue;
    }

    /* restore newline when writing */
    if (fputs(line, out) == EOF) {
      snprintf(err, err_len, "write error: %s", strerror(errno));
      free(line);
      fclose(in);
      return 0;
    }
    if (fputc('\n', out) == EOF) {
      snprintf(err, err_len, "write error: %s", strerror(errno));
      free(line);
      fclose(in);
      return 0;
    }
  }
  if (ferror(in)) {
    snprintf(err, err_len, "read error: %s", strerror(errno));
    free(line);
    fclose(in);
    return 0;
  }
  free(line);
  fclose(in);
  return 1;
}

int main(int argc, char **argv) {
  const char *in_path = NULL;
  const char *out_path = NULL;
  char **idirs = NULL;
  size_t n_idirs = 0u;
  size_t idirs_cap = 0u;
  char err[8192];
  const char *stack[MAX_STACK];
  int i;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--in") == 0) {
      if (i + 1 >= argc) {
        usage(stderr);
        return 2;
      }
      in_path = argv[++i];
      continue;
    }
    if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 >= argc) {
        usage(stderr);
        return 2;
      }
      out_path = argv[++i];
      continue;
    }
    if (strcmp(argv[i], "-I") == 0) {
      if (i + 1 >= argc) {
        usage(stderr);
        return 2;
      }
      if (n_idirs >= idirs_cap) {
        size_t nc = idirs_cap ? idirs_cap * 2u : 8u;
        char **nr = (char **)realloc(idirs, nc * sizeof(*nr));
        if (!nr) {
          fprintf(stderr, "olprep: oom\n");
          return 1;
        }
        idirs = nr;
        idirs_cap = nc;
      }
      idirs[n_idirs++] = argv[++i];
      continue;
    }
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(stdout);
      return 0;
    }
    fprintf(stderr, "olprep: unknown argument: %s\n", argv[i]);
    usage(stderr);
    return 2;
  }

  if (!in_path || !out_path) {
    usage(stderr);
    return 2;
  }

  {
    FILE *out = fopen(out_path, "wb");
    if (!out) {
      fprintf(stderr, "olprep: cannot open output %s: %s\n", out_path, strerror(errno));
      free(idirs);
      return 1;
    }
    if (!process_file(in_path, out, stack, 0, idirs, n_idirs, err, sizeof(err))) {
      fprintf(stderr, "olprep: %s\n", err);
      fclose(out);
      free(idirs);
      return 1;
    }
    if (fclose(out) != 0) {
      fprintf(stderr, "olprep: fclose output: %s\n", strerror(errno));
      free(idirs);
      return 1;
    }
  }
  free(idirs);
  return 0;
}
