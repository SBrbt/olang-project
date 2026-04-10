#ifndef OLANG_BACKEND_CODEGEN_X64_H
#define OLANG_BACKEND_CODEGEN_X64_H

#include "../ir.h"
#include "backend.h"

#include <stddef.h>

int ol_codegen_x64(const OlTargetInfo *ti, OlProgram *program, OobjObject *out, char *err, size_t err_len);

#endif
