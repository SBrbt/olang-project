#ifndef OLANG_BACKEND_BACKEND_H
#define OLANG_BACKEND_BACKEND_H

#include "../ir.h"
#include "../../../common/oobj.h"

#include <stddef.h>
#include <stdint.h>

typedef struct OlTargetInfo {
  char name[64];
  char arch[32];
  char abi[32];
  char ptr_type[16];
  uint8_t ptr_size;
  uint8_t stack_align;
  char call_reg0[8];
  char ret_reg0[8];
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
