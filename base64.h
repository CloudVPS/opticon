#ifndef _BASE64_H
#define _BASE64_H 1

#include <stdlib.h>
#include <string.h>

/* ============================= FUNCTIONS ============================= */

char *base64_encode (const char *src, size_t len, size_t *out_len);
char *base64_decode (const char *src, size_t len, size_t *out_len);

#endif
