#include "x64_reloc.h"

int x64_add_call_reloc(OobjObject *obj, uint64_t offset, const char *symbol_name) {
  return oobj_append_reloc(obj, 0, offset, symbol_name, OOBJ_RELOC_PC32, 0);
}

int x64_add_pc32_reloc(OobjObject *obj, uint32_t section_index, uint64_t offset, const char *symbol_name, int64_t addend) {
  return oobj_append_reloc(obj, section_index, offset, symbol_name, OOBJ_RELOC_PC32, addend);
}

int x64_add_pc64_reloc(OobjObject *obj, uint32_t section_index, uint64_t offset, const char *symbol_name, int64_t addend) {
  return oobj_append_reloc(obj, section_index, offset, symbol_name, OOBJ_RELOC_PC64, addend);
}
