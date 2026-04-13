#define _POSIX_C_SOURCE 200809L

#include "../common/oobj.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ERR_SZ 256

static int write_text_file(const char *path, const char *text) {
  FILE *f = fopen(path, "wb");
  size_t n;
  if (!f) return 0;
  n = strlen(text);
  if (n > 0 && fwrite(text, 1, n, f) != n) {
    fclose(f);
    return 0;
  }
  fclose(f);
  return 1;
}

static int with_temp_oobj(const char *text, int expect_ok, const char *err_substr) {
  char path[] = "/tmp/oobj-parse-XXXXXX";
  int fd = mkstemp(path);
  OobjObject obj;
  char err[ERR_SZ] = {0};
  int ok;
  int pass = 1;
  if (fd < 0) {
    fprintf(stderr, "oobj_parse_test: mkstemp failed\n");
    return 0;
  }
  close(fd);
  if (!write_text_file(path, text)) {
    unlink(path);
    fprintf(stderr, "oobj_parse_test: write failed\n");
    return 0;
  }
  oobj_init(&obj);
  ok = oobj_read_file(path, &obj, err, sizeof(err));
  if (expect_ok) {
    if (!ok) {
      fprintf(stderr, "oobj_parse_test: expected success, got error: %s\n", err);
      pass = 0;
    }
  } else {
    if (ok) {
      fprintf(stderr, "oobj_parse_test: expected failure but succeeded\n");
      pass = 0;
    } else if (err_substr && strstr(err, err_substr) == NULL) {
      fprintf(stderr, "oobj_parse_test: error mismatch, got: %s\n", err);
      pass = 0;
    }
  }
  oobj_free(&obj);
  unlink(path);
  return pass;
}

int main(void) {
  const char *ok_text =
      "OOBJv2\n"
      "target|x86_64-unknown-linux-gnu\n"
      "section|.text|16|6|90\n"
      "symbol|_start|0|0|global\n"
      "reloc|0|0|_start|abs64|0\n";

  const char *bad_section_align =
      "OOBJv2\n"
      "target|x86_64-unknown-linux-gnu\n"
      "section|.text|16x|6|90\n";

  const char *bad_symbol_value =
      "OOBJv2\n"
      "target|x86_64-unknown-linux-gnu\n"
      "section|.text|16|6|90\n"
      "symbol|_start|0|18446744073709551616|global\n";

  const char *bad_reloc_addend =
      "OOBJv2\n"
      "target|x86_64-unknown-linux-gnu\n"
      "section|.text|16|6|90\n"
      "reloc|0|0|_start|abs64|1x\n";

    const char *legacy_v1 =
      "OOBJv1\n"
      "target x86_64-unknown-linux-gnu\n"
      "section .text 16 6 90\n";

  if (!with_temp_oobj(ok_text, 1, NULL)) return 1;
  if (!with_temp_oobj(bad_section_align, 0, "bad section align")) return 1;
  if (!with_temp_oobj(bad_symbol_value, 0, "bad symbol value")) return 1;
  if (!with_temp_oobj(bad_reloc_addend, 0, "bad reloc addend")) return 1;
  if (!with_temp_oobj(legacy_v1, 0, "expected OOBJv2")) return 1;

  fprintf(stderr, "OK: oobj_parse_test\n");
  return 0;
}
