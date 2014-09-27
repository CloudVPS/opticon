#ifndef _TRANSPORT_UDP_H
#define _TRANSPORT_UDP_H 1

#include <libopticon/transport.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>

/* =============================== TYPES =============================== */

/** UDP implementation of outtransport */
typedef struct udp_outtransport_s {
    outtransport     super; /**< The transport */
    struct addrinfo *peeraddr; /**< Configured peer address */
    int              sock; /**< Socket to use */
} udp_outtransport;

/** UDP implementation of intransport */
typedef struct udp_intransport_s {
    intransport      super; /**< The transport */
    struct addrinfo *listenaddr; /**< IPvAny listen address */
    int              sock; /**< The bound socket */
} udp_intransport;

/* ============================= FUNCTIONS ============================= */

outtransport    *outtransport_create_udp (void);
intransport     *intransport_create_udp (void);

#endif
