#include <libopticon/transport_udp.h>
#include <libopticon/packetqueue.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main (int argc, const char *argv[]) {
    char sendbuf[1024], recvbuf[1024];
    struct sockaddr_storage peer;
    outtransport *out = outtransport_create_udp();
    intransport *in = intransport_create_udp();
    packetqueue *q;
    pktbuf *b;
    int i;
    
    assert (intransport_setlistenport (in, "0.0.0.0", 1874));
    assert (outtransport_setremote (out, "127.0.0.1", 1874));
    assert (q = packetqueue_create (256, in));
    
    for (i=0; i<96; ++i) {
        sprintf (sendbuf, "%i bottles of beer on the wall", i);
        assert (outtransport_send (out, sendbuf, 1024));
    }
    
    sleep (1);
    
    for (i=0; i<96; ++i) {
        sprintf (sendbuf, "%i bottles of beer on the wall", i);
        assert (b = packetqueue_waitpkt (q));
        assert (strcmp (sendbuf, (char *) b->pkt) == 0);
    }
    
    packetqueue_shutdown (q);
    intransport_close (in);
    outtransport_close (out);
    return 0;
}
