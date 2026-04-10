#ifndef OLANG_FRONTEND_SEMA_H
#define OLANG_FRONTEND_SEMA_H

#include "../ir.h"

#include <stddef.h>

int ol_sema_check(OlProgram *p, char *err, size_t err_len);

#endif
