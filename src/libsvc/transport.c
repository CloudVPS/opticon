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
int outtransport_send (outtransport *t, void *d, size_t sz) {
    return t->send (t, d, sz);
}

/** Close the outtransport and release its resources */
void outtransport_close (outtransport *t) {
    t->close (t);
    free (t);
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
size_t intransport_recv (intransport *t, void *into, size_t sz,
                         struct sockaddr_storage *client) {
    return t->recv (t, into, sz, client);
}

/** Close the intransport and release its resources */
void intransport_close (intransport *t) {
    t->close (t);
    free (t);
}
