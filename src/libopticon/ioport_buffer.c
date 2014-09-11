#include <libopticon/ioport_buffer.h>
#include <math.h>
#include <arpa/inet.h>

/** Read method of the buffer ioport */
int buffer_read (ioport *io, char *into, size_t sz) {
    bufferstorage *S = (bufferstorage *) io->storage;
    if (S->rpos + sz > S->bufsz) return 0;
    if (S->pos && (S->rpos + sz > S->pos)) return 0;
    memcpy (into, S->buf + S->rpos, sz);
    S->rpos += sz;
    return 1;
}

/** Write method of the buffer ioport */
int buffer_write (ioport *io, const char *dat, size_t sz) {
    bufferstorage *S = (bufferstorage *) io->storage;
    if (S->pos + sz > S->bufsz) return 0;
    if (S->buf + S->pos != dat) {
        memcpy (S->buf + S->pos, dat, sz);
    }
    S->pos += sz;
    return 1;
}

/** Close method of the buffer ioport */
void buffer_close (ioport *io) {
    free (io->storage);
    free (io);
}

/** Return number of bytes still available for reading */
size_t buffer_read_available (ioport *io) {
    bufferstorage *S = (bufferstorage *) io->storage;
    if (S->pos) return S->pos - S->rpos;
    return S->bufsz - S->rpos;
}

/** Return available buffer space */
size_t buffer_write_available (ioport *io) {
    bufferstorage *S = (bufferstorage *) io->storage;
    return S->bufsz - S->pos;
}

/** Return address of the buffer */
char *buffer_get_buffer (ioport *io) {
    bufferstorage *S = (bufferstorage *) io->storage;
    return S->buf;
}

/** Reset read position */
void buffer_reset_read (ioport *io) {
    bufferstorage *S = (bufferstorage *) io->storage;
    S->rpos = 0;
}

/** Create a buffer instance.
  * \param buf Backend buffer storage
  * \param sz The buffer's size
  * \return The freshly created ioport 
  */
ioport *ioport_create_buffer (char *buf, size_t sz) {
    ioport *res = (ioport *) malloc (sizeof (ioport));
    res->write = buffer_write;
    res->close = buffer_close;
    res->read = buffer_read;
    res->read_available = buffer_read_available;
    res->write_available = buffer_write_available;
    res->get_buffer = buffer_get_buffer;
    res->reset_read = buffer_reset_read;
    res->bitpos = 0;
    res->bitbuffer = 0;
    
    bufferstorage *S = res->storage = malloc (sizeof (bufferstorage));
    S->buf = buf;
    S->bufsz = sz;
    S->pos = 0;
    S->rpos = 0;
    return res;
}

