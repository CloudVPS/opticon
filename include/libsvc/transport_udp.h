#ifndef _TRANSPORT_UDP_H
#define _TRANSPORT_UDP_H 1

#include <libsvc/transport.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>

typedef struct udp_outtransport_s {
    outtransport     super;
    struct addrinfo *peeraddr;
} udp_outtransport;

typedef struct udp_intransport_s {
    intransport      super;
    struct addrinfo *listenaddr;
} udp_intransport;

outtransport *outtransport_udp_create (void);
intransport *intransport_udp_create (void);

#endif
