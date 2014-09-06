#ifndef _ENCODING_H
#define _ENCODING_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ioport_s;

typedef int (*writefunc)(struct ioport_s *, const char *, size_t);
typedef void (*closefunc)(struct ioport_s *);

typedef struct ioport_s {
    writefunc	 write;
    closefunc	 close;
    void 		*storage;
} ioport;

typedef struct bufferstorage_s {
    char *buf;
    size_t bufsz;
    unsigned int pos;
} bufferstorage;

ioport	*ioport_create_filereader (FILE *);
ioport	*ioport_create_buffer (char *buf, size_t sz);
void     ioport_close (ioport *e);
int      ioport_write (ioport *e, const char *data, size_t sz);

#endif


