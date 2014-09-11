#ifndef _IOPORT_BUFFER_H
#define _IOPORT_BUFFER_H 1

#include <libopticon/ioport.h>

/* =============================== TYPES =============================== */

/** Storage for buffer-type ioports */
typedef struct bufferstorage_s {
    char        *buf; /** The buffer, not ours. */
    size_t       bufsz; /** Size of the buffer */
    unsigned int pos; /** Write cursor */
    unsigned int rpos; /** Read cursor */
} bufferstorage;

/* ============================= FUNCTIONS ============================= */

ioport      *ioport_create_buffer (char *buf, size_t sz);

#endif
