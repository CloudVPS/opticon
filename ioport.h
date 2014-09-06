#ifndef _IOPORT_H
#define _IOPORT_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <datatypes.h>

/* =============================== TYPES =============================== */
struct ioport_s;

typedef int (*writefunc)(struct ioport_s *, const char *, size_t);
typedef void (*closefunc)(struct ioport_s *);

typedef struct ioport_s {
    writefunc	 write;
    closefunc	 close;
    void 		*storage;
    uint8_t		 bitpos;
    uint8_t		 bitbuffer;
} ioport;

typedef struct bufferstorage_s {
    char 		*buf;
    size_t		 bufsz;
    unsigned int pos;
} bufferstorage;

/* ============================= FUNCTIONS ============================= */

ioport	*ioport_create_filewriter (FILE *);
ioport	*ioport_create_buffer (char *buf, size_t sz);
void     ioport_close (ioport *io);
int      ioport_write (ioport *io, const char *data, size_t sz);
int		 ioport_write_uuid (ioport *io, uuid u);
int		 ioport_write_byte (ioport *io, uint8_t b);
int		 ioport_write_bits (ioport *io, uint8_t d, uint8_t numbits);
int		 ioport_flush_bits (ioport *io);

#endif


