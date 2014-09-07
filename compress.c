#include <compress.h>

int compress_data (ioport *in, ioport *out) {
    if (ioport_read_available (in) == 0) return 0;
    
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.total_out = 0;
    strm.next_in = (Bytef*)ioport_get_buffer (in);
    strm.avail_in = ioport_read_available (in);
    
    int r = deflateInit2 (&strm, Z_DEFAULT_COMPRESSION,
                          Z_DEFLATED, 15+16, 8,
                          Z_DEFAULT_STRATEGY);
    if (r != Z_OK) return 0;
    
    int st;
    do {
        strm.next_out = (Bytef*)ioport_get_buffer (out) + strm.total_out;
        strm.avail_out = ioport_write_available (out) - strm.total_out;
        st = deflate (&strm, Z_FINISH);
    } while (st == Z_OK);
    
    if (st != Z_STREAM_END) return 0;
    ioport_write (out, ioport_get_buffer (out), strm.total_out);
    return 1;
}

int decompress_data (ioport *in, ioport *out) {
    return 0;
}
