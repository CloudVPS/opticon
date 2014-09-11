#include <libopticon/ioport_file.h>
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

/** Available function for the filewriter ioport. Should be irrelevant,
    since it is only used for compression. */
size_t filewriter_available (ioport *io) {
    return 0;
}

/** Get buffer address. Defunct for filewriter */
char *filewriter_get_buffer (ioport *io) {
    return NULL;
}

/** Reset reading stream, not implemented */
void filewriter_reset_read (ioport *io) {
    return;
}

/** Write method (disabled for reader) */
int filereader_write (ioport *io, const char *dat, size_t sz) {
    return 0;
}

/** Read method */
int filereader_read (ioport *io, char *into, size_t sz) {
    FILE *F = (FILE *) io->storage;
    return (fread (into, sz, 1, F) > 0);
}

/** Reset stream (implemented with fseek) */
void filereader_reset_read (ioport *io) {
    FILE *F = (FILE *) io->storage;
    fseek (F, 0, SEEK_SET);
}

/** Create a filewriter instance.
  * \param F the FILE to connect
  * \return The freshly created ioport 
  */
ioport *ioport_create_filewriter (FILE *F) {
    ioport *res = (ioport *) malloc (sizeof (ioport));
    if (! res) return NULL;
    res->storage = F;
    res->write = filewriter_write;
    res->close = filewriter_close;
    res->read = filewriter_read;
    res->read_available = filewriter_available;
    res->write_available = filewriter_available;
    res->get_buffer = filewriter_get_buffer;
    res->reset_read = filewriter_reset_read;
    res->bitpos = 0;
    res->bitbuffer = 0;
    return res;
}

/** Create a filereader instance 
  * \param F the FILE to connect
  * \return The freshly created ioport
  */
ioport *ioport_create_filereader (FILE *F) {
    ioport *res = (ioport *) malloc (sizeof (ioport));
    res->storage = F;
    res->write = filereader_write;
    res->close = filewriter_close;
    res->read = filereader_read;
    res->read_available = filewriter_available;
    res->write_available = filewriter_available;
    res->get_buffer = filewriter_get_buffer;
    res->reset_read = filereader_reset_read;
    res->bitpos = 0;
    res->bitbuffer = 0;
    return res;
}

