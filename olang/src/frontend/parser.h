#ifndef OLANG_FRONTEND_PARSER_H
#define OLANG_FRONTEND_PARSER_H

#include "../ir.h"

#include <stddef.h>

int ol_parse_source_file(const char *path, OlProgram *out, char *err, size_t err_len);

#endif
