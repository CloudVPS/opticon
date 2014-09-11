#ifndef _CODEC_H
#define _CODEC_H 1

#include <libopticon/datatypes.h>
#include <libopticon/ioport.h>

/* =============================== TYPES =============================== */

typedef int (*encode_host_func)(ioport *, host *);
typedef int (*decode_host_func)(ioport *, host *);

/** Instance (bunch of function pointers) */
typedef struct codec_s {
    encode_host_func    encode_host;
    decode_host_func    decode_host;
} codec;

/* ============================= FUNCTIONS ============================= */

void         codec_release (codec *);
int          codec_encode_host (codec *, ioport *, host *);
int          codec_decode_host (codec *, ioport *, host *);

#endif
