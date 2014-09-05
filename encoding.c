#include <encoding.h>

int file_write (encoder *e, const char *dat, size_t sz) {
    FILE *F = (FILE *) e->storage;
    return (fwrite (dat, sz, 1, F) == sz);
}

void file_close (encoder *e) {
    free (e);
}

int buffer_write (encoder *e, const char *dat, size_t sz) {
    bufferstorage *S = (bufferstorage *) e->storage;
    if (S->pos + sz > S->bufsz) return 0;
    memcpy (S->buf + S->pos, dat, sz);
    S->pos += sz;
    return 1;
}

void buffer_close (encoder *e) {
    free (e->storage);
    free (e);
}

encoder *new_file_encoder (FILE *F) {
    encoder *res = (encoder *) malloc (sizeof (encoder));
    res->storage = F;
    res->write = file_write;
    res->close = file_close;
    return res;
}

encoder *new_buffer_encoder (char *buf, size_t sz) {
    encoder *res = (encoder *) malloc (sizeof (encoder));
    bufferstorage *S = res->storage = malloc (sizeof (bufferstorage));
    S->buf = buf;
    S->bufsz = sz;
    S->pos = 0;
    res->write = buffer_write;
    res->close = buffer_close;
    return res;
}

int encoder_write (encoder *e, const char *data, size_t sz) {
    return e->write (e, data, sz);
}

void encoder_close (encoder *e) {
    e->close (e);
}
