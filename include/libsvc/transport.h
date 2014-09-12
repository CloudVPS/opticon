#ifndef _TRANSPORT_H
#define _TRANSPORT_H 1

#include <stdlib.h>
#include <stddef.h>
#include <sys/socket.h>

/* =============================== TYPES =============================== */

struct outtransport_s; /* forward declaration */
struct intransport_s; /* forward declaration */

typedef int (*setremote_f)(struct outtransport_s *, const char *, int);
typedef int (*sendpkt_f)(struct outtransport_s *, void *, size_t);
typedef void (*outclose_f)(struct outtransport_s *);
typedef int (*setlistenport_f)(struct intransport_s *, const char *, int);
typedef size_t (*recvpkt_f)(struct intransport_s *, void *, size_t,
                            struct sockaddr_storage *);
typedef void (*inclose_f)(struct intransport_s *);

/** Abstract outtransport handle */
typedef struct outtransport_s {
    setremote_f     setremote; /**< Method */
    sendpkt_f       send; /**< Method */
    outclose_f      close; /**< Method */
} outtransport;

/** Abstract intransport handle */
typedef struct intransport_s {
    setlistenport_f setlistenport; /**< Method */
    recvpkt_f       recv; /**< Method */
    inclose_f       close; /**< Method */
} intransport;

/* ============================= FUNCTIONS ============================= */

int      outtransport_setremote (outtransport *t, const char *addr,
                                 int port);
int      outtransport_send (outtransport *t, void *d, size_t sz);
void     outtransport_close (outtransport *t);

int      intransport_setlistenport (intransport *t, const char *addr, 
                                    int port);
size_t   intransport_recv (intransport *t, void *into, size_t sz,
                           struct sockaddr_storage *client);
void     intransport_close (intransport *t);

#endif
