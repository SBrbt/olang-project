#ifndef KASM_ISA_H
#define KASM_ISA_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef enum IsaRelocKind {
  ISA_RELOC_NONE = 0,
  ISA_RELOC_PC32 = 1
} IsaRelocKind;

typedef struct IsaInstr {
  char mnemonic[64];
  uint8_t *bytes;
  size_t byte_len;
  uint32_t imm_bits;
  IsaRelocKind reloc_kind;
} IsaInstr;

typedef struct IsaSpec {
  IsaInstr *items;
  size_t count;
} IsaSpec;

void isa_free(IsaSpec *isa);
int isa_load(const char *path, IsaSpec *out, char *err, size_t err_len);
IsaInstr *isa_find(IsaSpec *isa, const char *mnemonic);

#endif
