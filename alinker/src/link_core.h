#ifndef TOOLCHAIN_ALINKER_LINK_CORE_H
#define TOOLCHAIN_ALINKER_LINK_CORE_H

#include "../../common/link_script.h"
#include "../../common/oobj.h"

#include <stddef.h>

/* verbose: if non-zero, print progress and layout numbers to stderr. */
int alinker_link_oobj_only(const LinkScript *script, const char *output_path, int verbose, char *err, size_t err_len);

#endif
