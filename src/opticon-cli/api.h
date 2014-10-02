#ifndef _API_H
#define _API_H 1

#include <libopticon/var.h>

/* ============================= FUNCTIONS ============================= */

var *api_call (const char *mth, var *data, const char *fmt, ...);
var *api_get (const char *fmt, ...);

#endif
