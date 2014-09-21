#include <libsvc/transport_udp.h>
#include <string.h>
#include <sys/socket.h>

/** Implementation of outtransport_setremote() */
int udp_outtransport_setremote (outtransport *t, const char *addr,
                                int port) {
    udp_outtransport *self = (udp_outtransport *) t;
    struct addrinfo hints;
    socklen_t sin_size;
    char portstr[16];
    
    if (self->peeraddr) free (self->peeraddr);
    
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    sprintf (portstr, "%i", port);
    
    if (getaddrinfo (addr, portstr, &hints, &(self->peeraddr)) < 0) {
        return 0;
    }
    
    self->sock = socket (self->peeraddr->ai_family, 
                         self->peeraddr->ai_socktype,
                         self->peeraddr->ai_protocol);
    
    if (self->sock < 0) return 0;
    return 1;
}

/** Implementation of outtransport_send() */
int udp_outtransport_send (outtransport *t, void *data, size_t sz) {
    udp_outtransport *self = (udp_outtransport *) t;
    
    if (sendto (self->sock, data, sz, 0,
                (struct sockaddr *) self->peeraddr->ai_addr,
                self->peeraddr->ai_addrlen) == 0) {
        return 0;
    }
    
    return 1;
}

/** Implementation of outtransport_close() */
void udp_outtransport_close (outtransport *t) {
    udp_outtransport *self = (udp_outtransport *) t;
    if (self->peeraddr) free (self->peeraddr);
    if (self->sock > 0) close (self->sock);
}

/** Create a UDP outtransport */
outtransport *outtransport_create_udp (void) {
    udp_outtransport *self = 
        (udp_outtransport *) malloc (sizeof (udp_outtransport));
    self->peeraddr = NULL;
    self->sock = -1;
    self->super.setremote = udp_outtransport_setremote;
    self->super.send = udp_outtransport_send;
    self->super.close = udp_outtransport_close;
    return (outtransport *) self;
}

/** Implementation of intransport_setlistenport() */
int udp_intransport_setlistenport (intransport *t, const char *addr,
                                   int port) {
    udp_intransport *self = (udp_intransport *) t;
    struct addrinfo hints;
    socklen_t sin_size;
    char portstr[16];
    
    if (self->listenaddr) free (self->listenaddr);
    
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;
    sprintf (portstr, "%i", port);
    
    if (getaddrinfo (addr, portstr, &hints, &(self->listenaddr)) < 0) {
        return 0;
    }
    
    self->sock = socket (self->listenaddr->ai_family, 
                         self->listenaddr->ai_socktype,
                         self->listenaddr->ai_protocol);
    
    if (bind (self->sock, (struct sockaddr *) self->listenaddr->ai_addr,
              self->listenaddr->ai_addrlen) < 0) {
        close (self->sock);
        return 0;
    }
    
    if (self->sock < 0) return 0;
    return 1;
}

/** Implementation of intranspot_recv() */
size_t udp_intransport_recv (intransport *t, void *into, size_t sz,
                             struct sockaddr_storage *client) {
    udp_intransport *self = (udp_intransport *) t;
    socklen_t rsize = sizeof (struct sockaddr_storage);
    return recvfrom (self->sock, into, sz, 0,
                     (struct sockaddr *) client, &rsize);
    return 0;
}

/** Implementation of intransport_close() */
void udp_intransport_close (intransport *t) {
    udp_intransport *self = (udp_intransport *) t;
    if (self->listenaddr) free (self->listenaddr);
    if (self->sock > 0) close (self->sock);
}

/** Create a UDP intransport */
intransport *intransport_create_udp (void) {
    udp_intransport *self =
        (udp_intransport *) malloc (sizeof (udp_intransport));
    self->listenaddr = NULL;
    self->sock = -1;
    self->super.setlistenport = udp_intransport_setlistenport;
    self->super.recv = udp_intransport_recv;
    self->super.close = udp_intransport_close;
    return (intransport *) self;
}
