#ifndef _TRANSPORT_H
#define _TRANSPORT_H 1

#include <stdlib.h>
#include <stddef.h>

/* =============================== TYPES =============================== */

struct outtransport_s; /* forward declaration */
struct intransport_s; /* forward declaration */

typedef int (*setremote_f)(struct outtransport_s *, const char *, int);
typedef int (*sendpkt_f)(struct outtransport_s *, void *, size_t);
typedef int (*setlistenport_f)(struct intransport_s *, const char *, int);
typedef size_t (*recvpkt_f)(struct intransport_s *, void *, size_t);

/** Abstract outtransport handle */
typedef struct outtransport_s {
    setremote_f     setremote; /**< Method */
    sendpkt_f       send; /**< Method */
} outtransport;

/** Abstract intransport handle */
typedef struct intransport_s {
    setlistenport_f setlistenport; /**< Method */
    recvpkt_f       recv; /**< Method */
} intransport;

/* ============================= FUNCTIONS ============================= */

int      outtransport_setremote (outtransport *t, const char *addr,
                                 int port);
int      outtransport_send (outtransport *t, void *d, size_t sz);
int      intransport_setlistenport (intransport *t, const char *addr, 
                                    int port);
size_t   intransport_recv (intransport *t, void *into, size_t sz);

#endif
