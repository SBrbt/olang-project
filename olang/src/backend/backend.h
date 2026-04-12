#ifndef OLANG_BACKEND_BACKEND_H
#define OLANG_BACKEND_BACKEND_H

#include "../ir.h"
#include "../../../common/oobj.h"

#include <stddef.h>
#include <stdint.h>

/* Backend selection uses `arch`; `name` is copied into the .oobj header. Other ABI details
 * live in the architecture backend (e.g. codegen_x64.c), not in this struct. */
typedef struct OlTargetInfo {
  char name[128];
  char arch[32];
} OlTargetInfo;

typedef struct OlEmitContext {
  OobjObject obj;
  uint8_t *text;
  size_t text_len;
} OlEmitContext;

typedef struct OlBackendVTable {
  const char *backend_name;
  int (*emit_program)(const OlTargetInfo *ti, OlProgram *program, OobjObject *out, char *err, size_t err_len);
} OlBackendVTable;

/* Select codegen backend for a target. New architectures register in target_registry and backend/. */
const OlBackendVTable *ol_backend_for_target(const OlTargetInfo *ti);

const OlTargetInfo *ol_target_x64_info(void);
const OlTargetInfo *ol_target_info_by_name(const char *name);
const OlBackendVTable *ol_backend_x64_vtable(void);

#endif
