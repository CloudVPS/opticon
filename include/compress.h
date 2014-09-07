#ifndef _COMPRESS_H
#define _COMPRESS_H 1

#include <datatypes.h>
#include <ioport.h>
#include <zlib.h>

/* ============================= FUNCTIONS ============================= */

int compress_data (ioport *in, ioport *out);
int decompress_data (ioport *in, ioport *out);

#endif
