#include "../backend.h"

#include <stdio.h>
#include <string.h>

static OlTargetInfo g_x64;
static int g_init = 0;

const OlTargetInfo *ol_target_x64_info(void) {
  if (!g_init) {
    memset(&g_x64, 0, sizeof(g_x64));
    snprintf(g_x64.name, sizeof(g_x64.name), "%s", "x86_64");
    snprintf(g_x64.arch, sizeof(g_x64.arch), "%s", "x86_64");
    g_init = 1;
  }
  return &g_x64;
}
