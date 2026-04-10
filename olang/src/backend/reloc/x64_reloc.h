#ifndef OLANG_BACKEND_X64_RELOC_H
#define OLANG_BACKEND_X64_RELOC_H

#include "../../../../common/oobj.h"

#include <stddef.h>
#include <stdint.h>

int x64_add_call_reloc(OobjObject *obj, uint64_t offset, const char *symbol_name);
int x64_add_pc32_reloc(OobjObject *obj, uint32_t section_index, uint64_t offset, const char *symbol_name, int64_t addend);
/* 64-bit PC-relative: patch = S + A - P (x86-64 R_X86_64_PC64 style). */
int x64_add_pc64_reloc(OobjObject *obj, uint32_t section_index, uint64_t offset, const char *symbol_name, int64_t addend);

#endif
