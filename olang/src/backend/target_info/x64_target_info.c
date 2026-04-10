#include "../backend.h"

#include <stdio.h>
#include <string.h>

static OlTargetInfo g_x64;
static int g_init = 0;

const OlTargetInfo *ol_target_x64_info(void) {
  if (!g_init) {
    memset(&g_x64, 0, sizeof(g_x64));
    snprintf(g_x64.name, sizeof(g_x64.name), "%s", "x86_64-linux-sysv-min");
    snprintf(g_x64.arch, sizeof(g_x64.arch), "%s", "x86_64");
    snprintf(g_x64.abi, sizeof(g_x64.abi), "%s", "sysv");
    snprintf(g_x64.ptr_type, sizeof(g_x64.ptr_type), "%s", "i64");
    g_x64.ptr_size = 8;
    g_x64.stack_align = 16;
    snprintf(g_x64.call_reg0, sizeof(g_x64.call_reg0), "%s", "rdi");
    snprintf(g_x64.ret_reg0, sizeof(g_x64.ret_reg0), "%s", "rax");
    g_init = 1;
  }
  return &g_x64;
}
