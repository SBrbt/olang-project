#include "backend.h"
#include "codegen_x64.h"
#include "../frontend/sema.h"

#include <stdio.h>
#include <string.h>

static int x64_emit_program(const OlTargetInfo *ti, OlProgram *program, OobjObject *out, char *err, size_t err_len) {
  if (!ol_sema_check(program, err, err_len)) return 0;
  return ol_codegen_x64(ti, program, out, err, err_len);
}

const OlBackendVTable *ol_backend_x64_vtable(void) {
  static OlBackendVTable vt = {
      .backend_name = "x64-module-backend",
      .emit_program = x64_emit_program,
  };
  return &vt;
}

const OlBackendVTable *ol_backend_for_target(const OlTargetInfo *ti) {
  if (!ti) return NULL;
  if (strcmp(ti->arch, "x86_64") == 0) return ol_backend_x64_vtable();
  return NULL;
}
