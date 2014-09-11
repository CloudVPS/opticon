#include <compress.h>

/** Compress data from a buffered ioport into another using
  * libz gzip compression.
  * \param in The input ioport
  * \param out The output ioport
  * \return 1 on success, 0 on failure
  */
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
    
    deflateEnd (&strm);
    if (st != Z_STREAM_END) return 0;

    ioport_write (out, ioport_get_buffer (out), strm.total_out);
    return 1;
}

/** Decompress data from a buffered ioport into another.
  * \param in The input ioport
  * \param out The output ioport
  * \return 1 on success, 0 on failure
  */
int decompress_data (ioport *in, ioport *out) {
    if (ioport_read_available (in) == 0) return 0;
    
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.total_out = 0;
    strm.next_in = (Bytef*)ioport_get_buffer (in);
    strm.avail_in = ioport_read_available (in);
    
    int r = inflateInit2 (&strm, 16+MAX_WBITS);
    if (r != Z_OK) return 0;
    
    int st;
    do {
        strm.next_out = (Bytef*)ioport_get_buffer (out) + strm.total_out;
        strm.avail_out = ioport_write_available (out) - strm.total_out;
        st = inflate (&strm, Z_FINISH);
    } while (st == Z_OK);

    inflateEnd (&strm);
    if (st != Z_STREAM_END) return 0;

    ioport_write (out, ioport_get_buffer (out), strm.total_out);
    return 1;
}
