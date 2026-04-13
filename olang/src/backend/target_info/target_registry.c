#include "../backend.h"

#include <string.h>

const OlTargetInfo *ol_target_info_by_name(const char *name) {
  const OlTargetInfo *x64 = ol_target_x64_info();
  if (name && strcmp(name, "x86_64") == 0) {
    return x64;
  }
  return NULL;
}
