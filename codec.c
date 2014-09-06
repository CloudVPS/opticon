#include <codec.h>
#include <stdlib.h>

codec *codec_create_json (void) {
    codec *res = (codec *) malloc (sizeof (codec));
    if (! res) return res;
    res->encode_host = jsoncodec_encode;
    res->decode_host = jsoncodec_decode;
    return res;
}

codec *codec_create_opticon (void) {
    return NULL;
}

int jsoncodec_encode (ioport *io, host *h) {
    dump_host_json (h, io);
    return 1;
}

int jsoncodec_decode (ioport *io, host *h) {
    return 0;
}
