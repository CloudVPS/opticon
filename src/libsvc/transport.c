#include <libsvc/transport.h>

/** Set up the remote host for a transport.
  * \param t The outtransport to connect.
  * \param addr The address to connect to (hostname or ip address).
  * \param port The port to connect to.
  * \ return 1 on success, 0 on failure.
  */
int outtransport_setremote (outtransport *t, const char *addr, int port) {
    return t->setremote (t, addr, port);
}

/** Send a packet to the remote host.
  * \param t The outtransport to use.
  * \param d The data to transmit.
  * \param sz The amount of data.
  * \return 1 on success, 0 on failure.
  */
int outtransport_sendpkt (outtransport *t, void *d, size_t sz) {
    return t->send (t, d, sz);
}

/** Set the port to listen to on an intransport.
  * \param t The intransport to use.
  * \param addr The address to listen to (hostname or ip address).
  * \param port The port to listen to.
  * \return 1 on success, 0 on failure.
  */
int intransport_setlistenport (intransport *t, const char *addr, int port) {
    return t->setlistenport (t, addr, port);
}

/** Receive a packet of data from the transport.
  * \param t The transport to receive from.
  * \param into The buffer to store the data in.
  * \param sz The size of the buffer.
  * \return The amount of data read (0 on failure).
  */
size_t intransport_recvpkt (intransport *t, void *into, size_t sz) {
    return t->recv (t, into, sz);
}
