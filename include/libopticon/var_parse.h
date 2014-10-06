#ifndef _PARSE_H
#define _PARSE_H 1

#include <libopticon/var.h>

/* ============================= FUNCTIONS ============================= */

int          var_load_json (var *, const char *);
int          var_parse_json (var *, const char *);
const char  *parse_error (void);

#endif
