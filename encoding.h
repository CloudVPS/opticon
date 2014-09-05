#ifndef _ENCODING_H
#define _ENCODING_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct encoder_s;

typedef int (*writefunc)(struct encoder_s *, const char *, size_t);
typedef void (*closefunc)(struct encoder_s *);

typedef struct encoder_s {
    writefunc write;
    closefunc close;
    void *storage;
} encoder;

typedef struct bufferstorage_s {
    char *buf;
    size_t bufsz;
    unsigned int pos;
} bufferstorage;

encoder *new_file_encoder (FILE *);
encoder *new_buffer_encoder (char *buf, size_t sz);
void     encoder_close (encoder *e);
int      encoder_write (encoder *e, const char *data, size_t sz);

#endif


