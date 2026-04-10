#ifndef TOOLCHAIN_OOBJ_H
#define TOOLCHAIN_OOBJ_H

#include <stddef.h>
#include <stdint.h>

typedef enum OobjRelocType {
  OOBJ_RELOC_ABS64 = 1,
  OOBJ_RELOC_PC32 = 2,
  OOBJ_RELOC_PC64 = 3
} OobjRelocType;

typedef struct OobjSection {
  char *name;
  uint32_t align;
  uint32_t flags;
  uint8_t *data;
  size_t data_len;
} OobjSection;

typedef struct OobjSymbol {
  char *name;
  int32_t section_index; /* -1 = undefined */
  uint64_t value;
  uint8_t is_global;
} OobjSymbol;

typedef struct OobjReloc {
  uint32_t section_index;
  uint64_t offset;
  char *symbol_name;
  OobjRelocType type;
  int64_t addend;
} OobjReloc;

typedef struct OobjObject {
  char target[128];
  OobjSection *sections;
  size_t section_count;
  OobjSymbol *symbols;
  size_t symbol_count;
  OobjReloc *relocs;
  size_t reloc_count;
} OobjObject;

void oobj_init(OobjObject *obj);
void oobj_free(OobjObject *obj);
int oobj_read_file(const char *path, OobjObject *out, char *err, size_t err_len);
int oobj_write_file(const char *path, const OobjObject *obj, char *err, size_t err_len);
int oobj_find_symbol(const OobjObject *obj, const char *name);

int oobj_append_section(OobjObject *obj, const char *name, uint32_t align, uint32_t flags, const uint8_t *data, size_t data_len);
int oobj_append_symbol(OobjObject *obj, const char *name, int32_t section_index, uint64_t value, int is_global);
int oobj_append_reloc(OobjObject *obj, uint32_t section_index, uint64_t offset, const char *symbol_name, OobjRelocType type, int64_t addend);

#endif
