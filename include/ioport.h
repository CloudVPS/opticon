#ifndef _IOPORT_H
#define _IOPORT_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <datatypes.h>

/* =============================== TYPES =============================== */
struct ioport_s; /* forward declaration */

typedef int (*writefunc)(struct ioport_s *, const char *, size_t);
typedef int (*readfunc)(struct ioport_s *, char *, size_t);
typedef void (*closefunc)(struct ioport_s *);
typedef size_t (*availfunc)(struct ioport_s *);
typedef char *(*bufferfunc)(struct ioport_s *);
typedef void (*resetfunc)(struct ioport_s *);

/** Instance of an ioport, bound to a specific implementation */
typedef struct ioport_s {
    writefunc    write; /** Method */
    closefunc    close; /** Method */
    readfunc     read; /** Method */
    availfunc    read_available; /** Method */
    availfunc    write_available; /** Method */
    bufferfunc   get_buffer; /** Method */
    resetfunc    reset_read; /** Method */
    void        *storage; /** Implementation-specific storage pointer */
    uint8_t      bitpos; /** Bits left on a buffered partial byte */
    uint8_t      bitbuffer; /** Accumulator for partial bytes in a bitstream*/
} ioport;

/** Storage for buffer-type ioports */
typedef struct bufferstorage_s {
    char        *buf; /** The buffer, not ours. */
    size_t       bufsz; /** Size of the buffer */
    unsigned int pos; /** Write cursor */
    unsigned int rpos; /** Read cursor */
} bufferstorage;

/* ============================= FUNCTIONS ============================= */

ioport      *ioport_create_filewriter (FILE *);
ioport      *ioport_create_filereader (FILE *);
ioport      *ioport_create_buffer (char *buf, size_t sz);
void         ioport_close (ioport *io);
char        *ioport_get_buffer (ioport *io);
void         ioport_reset_read (ioport *io);

size_t       ioport_write_available (ioport *io);
int          ioport_write (ioport *io, const char *data, size_t sz);
int          ioport_write_uuid (ioport *io, uuid u);
int          ioport_write_byte (ioport *io, uint8_t b);
int          ioport_write_bits (ioport *io, uint8_t d, uint8_t numbits);
int          ioport_flush_bits (ioport *io);
int          ioport_write_encstring (ioport *io, const char *str);
int          ioport_write_encfrac (ioport *io, double d);
int          ioport_write_encint (ioport *io, uint64_t i);
int          ioport_write_u64 (ioport *io, uint64_t i);

size_t       ioport_read_available (ioport *io);
int          ioport_read (ioport *io, char *into, size_t sz);
uuid         ioport_read_uuid (ioport *io);
uint8_t      ioport_read_byte (ioport *io);
uint8_t      ioport_read_bits (ioport *io, uint8_t numbits);
int          ioport_read_encstring (ioport *io, char *into);
double       ioport_read_encfrac (ioport *io);
uint64_t     ioport_read_encint (ioport *io);
uint64_t     ioport_read_u64 (ioport *io);

#endif
