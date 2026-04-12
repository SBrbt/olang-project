#include "../backend.h"

#include <stdio.h>
#include <string.h>

const OlTargetInfo *ol_target_info_by_name(const char *name) {
  const OlTargetInfo *x64 = ol_target_x64_info();
  static OlTargetInfo x64_alt;
  static int alt_init = 0;
  if (!name || !name[0] || strcmp(name, "x86_64") == 0 || strcmp(name, "x64") == 0) {
    return x64;
  }
  if (strcmp(name, "x86_64-alt") == 0) {
    if (!alt_init) {
      x64_alt = *x64;
      snprintf(x64_alt.name, sizeof(x64_alt.name), "%s", "x86_64-alt");
      alt_init = 1;
    }
    return &x64_alt;
  }
  return NULL;
}
