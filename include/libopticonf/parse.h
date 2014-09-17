#ifndef _PARSE_H
#define _PARSE_H 1

#include <libopticonf/var.h>

/* ============================= FUNCTIONS ============================= */

int          load_config (var *, const char *);
int          parse_config (var *, const char *);
const char  *parse_error (void);

#endif
