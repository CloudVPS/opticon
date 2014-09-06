#include <ioport.h>

int filewriter_write (ioport *e, const char *dat, size_t sz) {
    FILE *F = (FILE *) e->storage;
    return (fwrite (dat, sz, 1, F) == sz);
}

void filewriter_close (ioport *e) {
    free (e);
}

int buffer_write (ioport *e, const char *dat, size_t sz) {
    bufferstorage *S = (bufferstorage *) e->storage;
    if (S->pos + sz > S->bufsz) return 0;
    memcpy (S->buf + S->pos, dat, sz);
    S->pos += sz;
    return 1;
}

void buffer_close (ioport *e) {
    free (e->storage);
    free (e);
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
