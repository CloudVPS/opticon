#ifndef _CODEC_H
#define _CODEC_H 1

#include <datatypes.h>
#include <ioport.h>

/* =============================== TYPES =============================== */

typedef int (*encode_host_func)(ioport *, host *);
typedef int (*decode_host_func)(ioport *, host *);

/** Instance (bunch of function pointers) */
typedef struct codec_s {
    encode_host_func    encode_host;
    decode_host_func    decode_host;
} codec;

/* ============================= FUNCTIONS ============================= */

codec       *codec_create_json (void);
codec       *codec_create_pkt (void);
void         codec_release (codec *);
int          codec_encode_host (codec *, ioport *, host *);
int          codec_decode_host (codec *, ioport *, host *);

int          jsoncodec_encode_host (ioport *, host *);
int          jsoncodec_decode_host (ioport *, host *);

int          pktcodec_encode_host (ioport *, host *);
int          pktcodec_decode_host (ioport *, host *);

#endif
