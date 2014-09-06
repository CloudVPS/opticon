#ifndef _CODEC_H
#define _CODEC_H 1

#include <datatypes.h>
#include <ioport.h>

/* =============================== TYPES =============================== */

typedef int (*encode_host_func)(ioport *, host *);
typedef int (*decode_host_func)(ioport *, host *);

typedef struct codec_s {
    encode_host_func    encode_host;
    decode_host_func    decode_host;
} codec;

/* ============================= FUNCTIONS ============================= */

codec       *codec_create_json (void);
codec       *codec_create_opticon (void);

int          jsoncodec_encode (ioport *, host *);
int          jsoncodec_decode (ioport *, host *);

#endif
