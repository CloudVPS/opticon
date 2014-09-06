#include <codec.h>
#include <stdlib.h>

codec *codec_create_json (void) {
    codec *res = (codec *) malloc (sizeof (codec));
    if (! res) return res;
    res->encode_host = jsoncodec_encode_host;
    res->decode_host = jsoncodec_decode_host;
    return res;
}

codec *codec_create_opticon (void) {
    return NULL;
}

void codec_release (codec *c) {
    free (c);
}

int codec_encode_host (codec *c, ioport *io, host *h) {
    return c->encode_host (io, h);
}

int codec_decode_host (codec *c, ioport *io, host *h) {
    return c->decode_host (io, h);
}

int jsoncodec_encode_host (ioport *io, host *h) {
    dump_host_json (h, io);
    return 1;
}

int jsoncodec_decode_host (ioport *io, host *h) {
    return 0;
}
