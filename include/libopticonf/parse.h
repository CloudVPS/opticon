#ifndef _PARSE_H
#define _PARSE_H 1

#include <libopticonf/var.h>

/* ============================= FUNCTIONS ============================= */

int          load_json (var *, const char *);
int          parse_json (var *, const char *);
const char  *parse_error (void);

#endif
