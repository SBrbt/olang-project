#ifndef OLANG_IR_H
#define OLANG_IR_H

#include "frontend/ast.h"

#include <stddef.h>
#include <stdint.h>

typedef struct OlTypeDefStructField {
  char name[128];
  char type_name[128];
  uint32_t offset;
} OlTypeDefStructField;

typedef enum OlTypeDefKind {
  OL_TYPEDEF_STRUCT = 1,
  OL_TYPEDEF_ARRAY = 2
} OlTypeDefKind;

typedef struct OlTypeDef {
  OlTypeDefKind kind;
  char name[128];
  OlTypeDefStructField *fields;
  size_t field_count;
  char elem_type[128];
  uint32_t elem_count;
  uint32_t size_bytes;
} OlTypeDef;

struct OlProgram {
  char target_name[128];
  char entry_name[128];
  char source_path[512]; /* set by parser; CU hash for local fn link_name */
  OlExternDecl *externs;
  size_t extern_count;
  OlTypeDef *typedefs;
  size_t typedef_count;
  OlGlobalDef *globals;
  size_t global_count;
  OlFuncDef *funcs;
  size_t func_count;
  OlStringLit *strings;
  size_t string_count;
};

void ol_program_init(OlProgram *p);
void ol_program_free(OlProgram *p);

int ol_program_find_extern(const OlProgram *p, const char *name);
const OlExternDecl *ol_program_get_extern(const OlProgram *p, const char *name);
int ol_program_find_global(const OlProgram *p, const char *name);

int ol_program_find_typedef(const OlProgram *p, const char *name);
int ol_program_add_typedef_struct(OlProgram *p, const char *name);
int ol_program_add_typedef_struct_field(OlProgram *p, size_t type_idx, const char *field_name, const char *field_type);
int ol_program_add_typedef_array(OlProgram *p, const char *name, const char *elem_type, uint32_t count);

#endif
