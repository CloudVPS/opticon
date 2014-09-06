#include <ioport.h>
#include <math.h>

int filewriter_write (ioport *io, const char *dat, size_t sz) {
    FILE *F = (FILE *) io->storage;
    return (fwrite (dat, sz, 1, F) == sz);
}

void filewriter_close (ioport *io) {
    free (io);
}

int buffer_write (ioport *io, const char *dat, size_t sz) {
    bufferstorage *S = (bufferstorage *) io->storage;
    if (S->pos + sz > S->bufsz) return 0;
    memcpy (S->buf + S->pos, dat, sz);
    S->pos += sz;
    return 1;
}

void buffer_close (ioport *io) {
    free (io->storage);
    free (io);
}

ioport *ioport_create_filewriter (FILE *F) {
    ioport *res = (ioport *) malloc (sizeof (ioport));
    res->storage = F;
    res->write = filewriter_write;
    res->close = filewriter_close;
    res->bitpos = 0;
    res->bitbuffer = 0;
    return res;
}

ioport *ioport_create_buffer (char *buf, size_t sz) {
    ioport *res = (ioport *) malloc (sizeof (ioport));
    res->write = buffer_write;
    res->close = buffer_close;
    res->bitpos = 0;
    res->bitbuffer = 0;
    
    bufferstorage *S = res->storage = malloc (sizeof (bufferstorage));
    S->buf = buf;
    S->bufsz = sz;
    S->pos = 0;
    return res;
}

int ioport_write (ioport *io, const char *data, size_t sz) {
    if (io->bitpos) {
        if (! ioport_flush_bits (io)) return 0;
    }
    return io->write (io, data, sz);
}

int ioport_write_byte (ioport *io, uint8_t b) {
    if (io->bitpos) {
        if (! ioport_flush_bits (io)) return 0;
    }
    return io->write (io, (const char *) &b, 1);
}

int ioport_write_uuid (ioport *io, uuid u) {
    return 0;
}

void ioport_close (ioport *io) {
    io->close (io);
}

uint8_t BITMASKS[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

int ioport_write_bits (ioport *io, uint8_t d, uint8_t bits) {
    if (bits>8) return 0;
    uint8_t bits1 = (io->bitpos + bits < 8) ? bits : 8-io->bitpos;
    uint8_t bits2 = bits - bits1;
    uint8_t shift = (8 - io->bitpos) - bits1;
    uint8_t data1 = (d & BITMASKS[bits1]) << shift;
    uint8_t data2 = (d >> bits1) & BITMASKS[bits2];
    io->bitbuffer |= data1;
    io->bitpos += bits1;
    if (io->bitpos > 7) {
        if (! io->write (io, (const char *)&(io->bitbuffer), 1)) return 0;
        io->bitpos = bits2;
        io->bitbuffer = bits2 ? data2 << (8-bits2) : 0;
    }
    return 1;
}

int ioport_flush_bits (ioport *io) {
    if (! io->bitpos) return 1;
    if (! io->write (io, (const char *)&(io->bitbuffer), 1)) return 0;
    io->bitpos = io->bitbuffer = 0;
    return 1;
}

static const char *ENCSET = "abcdefghijklmnopqrstuvwxyz#/-_"
                            " 0123456789ABCDEFGHJKLMNPQSTUVWXZ";

static uint8_t DECSET[128] = {
    63,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    30,255,255,26,255,255,255,255,255,255,255,255,255,28,255,27,31,
    32,33,34,35,36,37,38,39,40,255,255,255,255,255,255,255,41,42,43,
    44,45,46,47,48,255,49,50,51,52,53,255,54,55,255,56,57,58,59,60,61,
    255,62,255,255,255,255,29,255,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,255,255,255,255
};

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
    for (i=0;i<len;++i) {
        if (! ioport_write_bits (io, DECSET[str[i]], 6)) return 0;
    }
    return ioport_flush_bits (io);
}

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
