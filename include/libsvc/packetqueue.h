#ifndef _PACKETQUEUE_H
#define _PACKETQUEUE_H 1

#include <libsvc/thread.h>
#include <libsvc/transport.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

/* =============================== TYPES =============================== */

typedef struct pktbuf_s {
    uint8_t                  pkt[4096];
    struct sockaddr_storage  addr;
} pktbuf;

typedef struct packetqueue_s {
    thread               super;
    pthread_mutexattr_t  mattr;
    pthread_mutex_t      mutex;
    pthread_cond_t       cond;
    intransport         *trans;
    pktbuf              *buffer;
    size_t               sz;
    volatile size_t      rpos;
    volatile size_t      wpos;
} packetqueue;

/* ============================= FUNCTIONS ============================= */

thread *packetqueue_create (size_t qcount, intransport *producer);

#endif
