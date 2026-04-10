#ifndef KASM_ASM_H
#define KASM_ASM_H

#include "kasm_isa.h"

#include "../../common/oobj.h"

#include <stdio.h>

/* Returns 0 on success, 4 on OOM, 5 on assembly error, 6 on write error. Caller owns isa/obj; on success obj has .text appended. */
int kasm_compile(IsaSpec *isa, const char *input_path, const char *output_path, OobjObject *obj);

#endif
