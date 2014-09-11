#include <libopticon/codec.h>
#include <libopticon/util.h>
#include <stdlib.h>

/** Destroy a codec */
void codec_release (codec *c) {
    free (c);
}

/** Call on a codec to encode a host's data into an ioport */
int codec_encode_host (codec *c, ioport *io, host *h) {
    return c->encode_host (io, h);
}

/** Call on a codec to load host data from an ioport */
int codec_decode_host (codec *c, ioport *io, host *h) {
    return c->decode_host (io, h);
}

