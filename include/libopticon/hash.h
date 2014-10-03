#ifndef _HASH_H
#define _HASH_H 1

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>

uint32_t hash_token (const char *);

#endif
