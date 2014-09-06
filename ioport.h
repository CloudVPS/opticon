#ifndef _IOPORT_H
#define _IOPORT_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <datatypes.h>

/* =============================== TYPES =============================== */
struct ioport_s;

typedef int (*writefunc)(struct ioport_s *, const char *, size_t);
typedef int (*readfunc)(struct ioport_s *, char *, size_t);
typedef void (*closefunc)(struct ioport_s *);

typedef struct ioport_s {
    writefunc    write;
    closefunc    close;
    readfunc     read;
    void        *storage;
    uint8_t      bitpos;
    uint8_t      bitbuffer;
} ioport;

typedef struct bufferstorage_s {
    char        *buf;
    size_t       bufsz;
    unsigned int pos;
    unsigned int rpos;
} bufferstorage;

/* ============================= FUNCTIONS ============================= */

ioport      *ioport_create_filewriter (FILE *);
ioport      *ioport_create_buffer (char *buf, size_t sz);
void         ioport_close (ioport *io);

int          ioport_write (ioport *io, const char *data, size_t sz);
int          ioport_write_uuid (ioport *io, uuid u);
int          ioport_write_byte (ioport *io, uint8_t b);
int          ioport_write_bits (ioport *io, uint8_t d, uint8_t numbits);
int          ioport_flush_bits (ioport *io);
int          ioport_write_encstring (ioport *io, const char *str);
int          ioport_write_encfrac (ioport *io, double d);
int          ioport_write_encint (ioport *io, uint64_t i);
int          ioport_write_u64 (ioport *io, uint64_t i);

int          ioport_read (ioport *io, char *into, size_t sz);
uuid         ioport_read_uuid (ioport *io);
uint8_t      ioport_read_byte (ioport *io);
uint8_t      ioport_read_bits (ioport *io, uint8_t numbits);
int          ioport_read_encstring (ioport *io, char *into);
double       ioport_read_encfrac (ioport *io);
uint64_t     ioport_read_encint (ioport *io);
uint64_t     ioport_read_u64 (ioport *io);

#endif

