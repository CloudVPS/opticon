#include <ioport.h>
#include <math.h>
#include <arpa/inet.h>

/** Write method of the filewriter ioport */
int filewriter_write (ioport *io, const char *dat, size_t sz) {
    FILE *F = (FILE *) io->storage;
    return (fwrite (dat, sz, 1, F) > 0);
}

/** Read method of the filewriter ioport (defunct) */
int filewriter_read (ioport *io, char *into, size_t sz) {
    return 0;
}

/** Close method of the filewriter ioport */
void filewriter_close (ioport *io) {
    free (io);
}

/** Write method of the buffer ioport */
int buffer_write (ioport *io, const char *dat, size_t sz) {
    bufferstorage *S = (bufferstorage *) io->storage;
    if (S->pos + sz > S->bufsz) return 0;
    memcpy (S->buf + S->pos, dat, sz);
    S->pos += sz;
    return 1;
}

/** Read method of the buffer ioport */
int buffer_read (ioport *io, char *into, size_t sz) {
    bufferstorage *S = (bufferstorage *) io->storage;
    if (S->rpos + sz > S->bufsz) return 0;
    if (S->pos && (S->rpos + sz > S->pos)) return 0;
    memcpy (into, S->buf + S->rpos, sz);
    S->rpos += sz;
    return 1;
}

/** Close method of the buffer ioport */
void buffer_close (ioport *io) {
    free (io->storage);
    free (io);
}

/** Create a filewriter instance.
    \param F the FILE to connect
    \return The freshly created ioport */
ioport *ioport_create_filewriter (FILE *F) {
    ioport *res = (ioport *) malloc (sizeof (ioport));
    res->storage = F;
    res->write = filewriter_write;
    res->close = filewriter_close;
    res->read = filewriter_read;
    res->bitpos = 0;
    res->bitbuffer = 0;
    return res;
}

/** Create a buffer instance.
    \param buf Backend buffer storage
    \param sz The buffer's size
    \return The freshly created ioport */
ioport *ioport_create_buffer (char *buf, size_t sz) {
    ioport *res = (ioport *) malloc (sizeof (ioport));
    res->write = buffer_write;
    res->close = buffer_close;
    res->read = buffer_read;
    res->bitpos = 0;
    res->bitbuffer = 0;
    
    bufferstorage *S = res->storage = malloc (sizeof (bufferstorage));
    S->buf = buf;
    S->bufsz = sz;
    S->pos = 0;
    S->rpos = 0;
    return res;
}

/** Write a blob of data to an ioport.
    \param io The ioport to write to
    \param data The blob
    \param sz The blob size
    \return 1 on success, 0 on failure */
int ioport_write (ioport *io, const char *data, size_t sz) {
    if (io->bitpos) {
        if (! ioport_flush_bits (io)) return 0;
    }
    return io->write (io, data, sz);
}

/** Write a single byte to an ioport.
    \param io The ioport
    \param b The byte to write
    \return 1 on success, 0 on failure */
int ioport_write_byte (ioport *io, uint8_t b) {
    if (io->bitpos) {
        if (! ioport_flush_bits (io)) return 0;
    }
    return io->write (io, (const char *) &b, 1);
}

/** Write a raw uuid to an ioport.
    \param io The ioport
    \param u The uuid
    \return 1 on success, 0 on failure */
int ioport_write_uuid (ioport *io, uuid u) {
    return 0;
}

/** Close an ioport */
void ioport_close (ioport *io) {
    io->close (io);
}

uint8_t BITMASKS[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

/** Write a specific number of bits to an ioport. Subsequent calls
    will continue at the last bit offset. Bits are put into the
    stream little-endian.
    \param io The ioport
    \param d The data
    \param bits The number of bits to write (max 8) */
int ioport_write_bits (ioport *io, uint8_t d, uint8_t bits) {
    if (bits>8) return 0;
    uint8_t bits1 = (io->bitpos + bits < 8) ? bits : 8-io->bitpos;
    uint8_t bits2 = bits - bits1;
    uint8_t shift = (8 - io->bitpos) - bits1;
    uint8_t data1 = ((d >> bits2) & (BITMASKS[bits1])) << shift;
    uint8_t data2 = d & BITMASKS[bits2];
    
    io->bitbuffer |= data1;
    io->bitpos += bits1;
    if (io->bitpos > 7) {
        if (! io->write (io, (const char *)&(io->bitbuffer), 1)) {
            return 0;
        }
        io->bitpos = bits2;
        io->bitbuffer = bits2 ? data2 << (8-bits2) : 0;
    }
    return 1;
}

/** Flush any outstanding bits left over from earlier calls to
    ioport_write_bits(). This final byte will be 0-padded.
    \param io The ioport.
    \return 1 on success, 0 on failure. */
int ioport_flush_bits (ioport *io) {
    if (! io->bitpos) return 1;
    if (! io->write (io, (const char *)&(io->bitbuffer), 1)) return 0;
    io->bitpos = io->bitbuffer = 0;
    return 1;
}

static const char *ENCSET = "abcdefghijklmnopqrstuvwxyz#/-_"
                            " 0123456789ABCDEFGHJKLMNPQSTUVWXZ.";

static uint8_t DECSET[128] = {
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    30,255,255,26,255,255,255,255,255,255,255,255,255,28,63,27,31,
    32,33,34,35,36,37,38,39,40,255,255,255,255,255,255,255,41,42,43,
    44,45,46,47,48,255,49,50,51,52,53,255,54,55,255,56,57,58,59,60,61,
    255,62,255,255,255,255,29,255,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,255,255,255,255
};

/** Write an encoded string to an ioport. The string will be written
    as either a plain pascal string, or a stream of 6-bit characters.
    \param io The ioport
    \param str The string to encode (maximum length 127 chars)
    \return 1 on success, 0 on failure */
int ioport_write_encstring (ioport *io, const char *str) {
    uint8_t i;
    char out_ascii[128];
    size_t outsz = 0;
    size_t len = strlen (str);
    if (len > 127) return 0;
    for (i=0;i<len;++i) {
        if ((str[i]>127) || (DECSET[str[i]] == 255)) {
            if (! ioport_write_byte (io, len)) return 0;
            return ioport_write (io, str, len);
        }
    }
    if (! ioport_write_byte (io, len | 0x80)) return 0;
    for (i=0;i<len;++i) {
        if (! ioport_write_bits (io, DECSET[str[i]], 6)) return 0;
    }
    return ioport_flush_bits (io);
}

/** Encode a floating point into an ioport as a 16-bit fixed point
    number.
    \param io The ioport
    \param d The value to encode (between 0.0 and 255.999)
    \return 1 on success, 0 on failure */
int ioport_write_encfrac (ioport *io, double d) {
    if (d <= 0.0) return ioport_write (io, "\0\0", 2);
    if (d >= 255.999) {
        return ioport_write_byte (io,255) &&
               ioport_write_byte (io,255);
    }
    double fl = floor (d);
    double fr = (d - fl) * 255;
    if (! ioport_write_byte (io, (uint8_t) fl)) return 0;
    return ioport_write_byte (io, (uint8_t) fr);
}

/** Encode a 63 bits unsigned integer into an ioport.
    \param io The iobuffer
    \param i The integer to encode
    \return 1 on success, 0 on failure */
int ioport_write_encint (ioport *io, uint64_t i) {
    uint64_t msk;
    uint8_t byte;
    uint8_t started = 0;
    for (int bitpos=56;bitpos>=0;bitpos-=7) {
        msk = 0x7f << bitpos;
        byte = (i & msk) >> bitpos;
        if (byte || started) {
            if (bitpos) byte |= 0x80;
            started = 1;
            if (! ioport_write_byte (io, byte)) return 0;
        }
    }
    return 1;
}

/** Write a raw 64 bits unsigned integer to an ioport in an endian-
    safe fashion.
    \param io The ioport
    \param i The integer to encode
    \return 1 on success, 0 on failure */
int ioport_write_u64 (ioport *io, uint64_t i) {
    uint64_t netorder = ((uint64_t) htonl (i&0xffffffffLLU)) << 32;
    netorder |= htonl ((i & 0xffffffff00000000LLU) >> 32);
    return ioport_write (io, (const char *)&netorder, sizeof (netorder));
}

/** Read data from an ioport.
    \param io The ioport
    \param into The buffer to read to
    \param sz The number of bytes to read
    \return 1 on success, 0 on failure */
int ioport_read (ioport *io, char *into, size_t sz) {
    io->bitpos = io->bitbuffer = 0;
    return io->read (io, into, sz);
}

/** Read a uuid from an ioport */
uuid ioport_read_uuid (ioport *io) {
    uuid res = {0,0};
    return res;
}

/** Read a byte from an ioport */
uint8_t ioport_read_byte (ioport *io) {
    uint8_t res = 0;
    ioport_read (io, (char *)&res, 1);
    return res;
}

/** Read a bitstream out of an ioport */
uint8_t ioport_read_bits (ioport *io, uint8_t numbits) {
    if (numbits > 8) return 0;
    if (! numbits) return 0;
    if (! io->bitpos) io->bitbuffer = ioport_read_byte (io);
    uint8_t res = 0;
    uint8_t bits1 = (numbits + io->bitpos > 8) ? 8-io->bitpos : numbits;
    uint8_t shift = (8 - io->bitpos) - bits1;
    uint8_t mask1 = BITMASKS[bits1] << shift;
    uint8_t bits2 = numbits - bits1;
    uint8_t shift2 = 8-bits2;
    uint8_t mask2 = bits2 ? BITMASKS[bits2] << shift2 : 0;

    res = (io->bitbuffer & mask1) >> shift << bits2;
    if (bits2) {
        io->bitbuffer = ioport_read_byte (io);
        res |= (io->bitbuffer & mask2) >> shift2;
        io->bitpos = bits2;
    }
    else io->bitpos += bits1;
    if (io->bitpos > 8) io->bitpos = 0;
    return res;
}

/** Read an encoded string from an ioport.
    \param io The ioport
    \param into The string buffer (size 128). */
int ioport_read_encstring (ioport *io, char *into) {
    io->bitpos = io->bitbuffer = 0;
    uint8_t sz = ioport_read_byte (io);
    if (! sz) return 0;
    
    if (sz < 0x80) {
        if (! ioport_read (io, into, sz)) return 0;
        into[sz] = '\0';
        return 1;
    }
    
    char *crsr = into;
    sz = sz & 0x7f;
    uint8_t c =0;
    
    
    for (uint8_t i=0; i<sz; ++i) {
        c = ioport_read_bits (io, 6);
        *crsr++ = ENCSET[c];
    }
    if (sz < 127) *crsr = '\0';
    else into[127] = '\0';
    return 1;
}

/** Read a raw unsigned 64 bits integer from an ioport */
uint64_t ioport_read_u64 (ioport *io) {
    io->bitpos = io->bitbuffer = 0;
    uint64_t dt;
    if (! ioport_read (io, (char *) &dt, sizeof (dt))) return 0;
    uint64_t res;
    res = ((uint64_t) ntohl (dt & 0xffffffffLLU)) << 32;
    res |= ntohl ((dt & 0xffffffff00000000LLU) >> 32);
    return res;
}

/** Read an encoded 63 bits integer from an ioport */
uint64_t ioport_read_encint (ioport *io) {
    uint64_t res = 0ULL;
    uint64_t d;
    
    do {
        d = (uint64_t) ioport_read_byte (io);
        if (res) res <<= 7;
        res |= (d & 0x7f);
    } while (d & 0x80);
    
    return res;
}

/** Read an encoded fractional number from an ioport */
double ioport_read_encfrac (ioport *io) {
    double res = 0;
    res = ioport_read_byte (io);
    res += ((double)ioport_read_byte(io))/256.0;
    return res;
}

