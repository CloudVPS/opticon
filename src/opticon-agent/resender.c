#include "opticon-agent.h"
#include <libopticon/ioport.h>
#include <libopticon/codec_pkt.h>
#include <libopticon/pktwrap.h>
#include <libopticon/util.h>
#include <libopticon/var.h>
#include <libopticon/var_parse.h>
#include <libopticon/react.h>
#include <libopticon/daemon.h>
#include <libopticon/log.h>
#include <libopticon/cliopt.h>
#include <libopticon/transport_udp.h>
#include <arpa/inet.h>

/** Main thread runner. Waits for the condtional to fire, then does a 50% random
  * throw to determine whether to resend at all. If so, the thread sleeps for a
  * random time between 5 and 20 seconds, then resends the packet.
  * \param t Underlying thread.
  */
void authresender_run (thread *t) {
    authresender *self = (authresender *) t;
    while (1) {
        conditional_wait_fresh (self->cond);
        if (rand()&1) {
            sleep ((rand() & 15)+5);
            log_debug ("Resending auth packet");
            outtransport_send (self->trans, (void*) self->buf.pkt,
                               self->buf.sz);
        }
    }
}

/** Initialize and start the authresender thread */
authresender *authresender_create (outtransport *t) {
    authresender *self = (authresender *) malloc (sizeof (authresender));
    self->cond = conditional_create();
    self->trans = t;
    memset (&self->buf, 0, sizeof (pktbuf));
    thread_init (&self->super, authresender_run, NULL);
    return self;
}

/** Place packet data in the authresender buffer and trigger the conditional
  * to schedule a potential resend.
  * \param self Pointer to the authresender
  * \param dt The data to resend
  * \param sz The size of the data
  */
void authresender_schedule (authresender *self, const void *dt, size_t sz) {
    memcpy (&self->buf.pkt, dt, sz);
    self->buf.sz = sz;
    conditional_signal (self->cond);
}
