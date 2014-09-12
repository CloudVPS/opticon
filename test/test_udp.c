#include <libsvc/transport_udp.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main (int argc, const char *argv[]) {
    char sendbuf[1024], recvbuf[1024];
    struct sockaddr_storage peer;
    outtransport *out = outtransport_create_udp();
    intransport *in = intransport_create_udp();
    strcpy (sendbuf, "Hello, world.\n");
    
    assert (intransport_setlistenport (in, "0.0.0.0", 1874));
    assert (outtransport_setremote (out, "127.0.0.1", 1874));
    assert (outtransport_send (out, sendbuf, 1024));
    assert (intransport_recv (in, recvbuf, 1024, &peer));
    assert (strcmp (sendbuf, recvbuf) == 0);
    intransport_close (in);
    outtransport_close (out);
    return 0;
}
