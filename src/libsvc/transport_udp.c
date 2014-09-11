#include <libsvc/transport_udp.h>

/** Implementation of outtransport_setremote() */
int udp_outtransport_setremote (outtransport *t, const char *addr,
                                int port) {
    udp_outtransport *self = (udp_outtransport *) t;
    return 0;
}

/** Implementation of outtransport_send() */
int udp_outtransport_send (outtransport *t, void *data, size_t sz) {
    udp_outtransport *self = (udp_outtransport *) t;
    return 0;
}

/** Create a UDP outtransport */
outtransport *outtransport_udp_create (void) {
    udp_outtransport *self = 
        (udp_outtransport *) malloc (sizeof (udp_outtransport));
    self->peeraddr = NULL;
    self->super.setremote = udp_outtransport_setremote;
    self->super.send = udp_outtransport_send;
    return (outtransport *) self;
}

/** Implementation of intransport_setlistenport() */
int udp_intransport_setlistenport (intransport *t, const char *addr,
                                   int port) {
    udp_intransport *self = (udp_intransport *) t;
    return 0;
}

/** Implementation of intranspot_recv() */
size_t udp_intransport_recv (intransport *t, void *into, size_t sz) {
    udp_intransport *self = (udp_intransport *) t;
    return 0;
}

/** Create a UDP intransport */
intransport *intransport_udp_create (void) {
    udp_intransport *self =
        (udp_intransport *) malloc (sizeof (udp_intransport));
    self->listenaddr = NULL;
    self->super.setlistenport = udp_intransport_setlistenport;
    self->super.recv = udp_intransport_recv;
    return (intransport *) self;
}
