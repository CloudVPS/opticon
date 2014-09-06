#include <encoding.h>

int filereader_write (ioport *e, const char *dat, size_t sz) {
    FILE *F = (FILE *) e->storage;
    return (fwrite (dat, sz, 1, F) == sz);
}

void filereader_close (ioport *e) {
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

ioport *ioport_create_filereader (FILE *F) {
    ioport *res = (ioport *) malloc (sizeof (ioport));
    res->storage = F;
    res->write = filereader_write;
    res->close = filereader_close;
    return res;
}

ioport *ioport_create_buffer (char *buf, size_t sz) {
    ioport *res = (ioport *) malloc (sizeof (ioport));
    bufferstorage *S = res->storage = malloc (sizeof (bufferstorage));
    S->buf = buf;
    S->bufsz = sz;
    S->pos = 0;
    res->write = buffer_write;
    res->close = buffer_close;
    return res;
}

int ioport_write (ioport *e, const char *data, size_t sz) {
    return e->write (e, data, sz);
}

void ioport_close (ioport *e) {
    e->close (e);
}
