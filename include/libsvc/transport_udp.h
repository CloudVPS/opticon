#ifndef _TRANSPORT_UDP_H
#define _TRANSPORT_UDP_H 1

#include <libsvc/transport.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>

/* =============================== TYPES =============================== */

typedef struct udp_outtransport_s {
    outtransport     super;
    struct addrinfo *peeraddr;
    int              sock;
} udp_outtransport;

typedef struct udp_intransport_s {
    intransport      super;
    struct addrinfo *listenaddr;
    int              sock;
} udp_intransport;

/* ============================= FUNCTIONS ============================= */

outtransport    *outtransport_create_udp (void);
intransport     *intransport_create_udp (void);

#endif
