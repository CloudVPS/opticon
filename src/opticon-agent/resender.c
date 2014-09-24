#include "opticon-agent.h"
#include <libopticon/ioport.h>
#include <libopticon/codec_pkt.h>
#include <libopticon/pktwrap.h>
#include <libopticon/util.h>
#include <libopticonf/var.h>
#include <libopticonf/parse.h>
#include <libopticonf/react.h>
#include <libsvc/daemon.h>
#include <libsvc/log.h>
#include <libsvc/cliopt.h>
#include <libsvc/transport_udp.h>
#include <arpa/inet.h>

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

authresender *authresender_create (outtransport *t) {
    authresender *self = (authresender *) malloc (sizeof (authresender));
    self->cond = conditional_create();
    self->trans = t;
    memset (&self->buf, 0, sizeof (pktbuf));
    thread_init (&self->super, authresender_run, NULL);
    return self;
}

void authresender_schedule (authresender *self, const void *dt, size_t sz) {
    memcpy (&self->buf.pkt, dt, sz);
    self->buf.sz = sz;
    conditional_signal (self->cond);
}
