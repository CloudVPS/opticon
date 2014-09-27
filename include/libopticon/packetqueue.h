#ifndef _PACKETQUEUE_H
#define _PACKETQUEUE_H 1

#include <libopticon/thread.h>
#include <libopticon/transport.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

/* =============================== TYPES =============================== */

/** Storage for a received packet. An array of these babies is used
  * in the packetqueue. 
  */
typedef struct pktbuf_s {
    uint8_t                  pkt[2048]; /**< Them bytes */
    struct sockaddr_storage  addr; /**< Address we received it from */
    size_t                   sz; /**< Payload size */
} pktbuf;

/** A circular buffer queue for incoming packets. A background thread
  * receives the packets and places them in the queue. A single
  * consumer can take in the packets at will.
  */
typedef struct packetqueue_s {
    thread               super; /**< Thread infrastructure */
    conditional         *cond; /**< Conditional triggered on new packets */
    intransport         *trans; /**< Transport used by the receive thread */
    pktbuf              *buffer; /**< Array of packet buffers */
    size_t               sz; /**< Number of elements in the array */
    volatile size_t      rpos; /**< Read cursor */
    volatile size_t      wpos; /**< Write cursor */
} packetqueue;

/* ============================= FUNCTIONS ============================= */

packetqueue *packetqueue_create (size_t qcount, intransport *producer);
pktbuf *packetqueue_waitpkt (packetqueue *t);
void packetqueue_shutdown (packetqueue *t);

#endif
