#include <libsvc/packetqueue.h>
#include <libsvc/thread.h>
#include <libsvc/transport.h>
#include <libsvc/log.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/** Producer thread for a packetqueue. Receives packets from the intransport
  * and puts them on the ringbuffer.
  */
void packetqueue_run (thread *t) {
    packetqueue *self = (packetqueue *) t;
    int errcnt = 0;
    while (1) {
        void *daddr = self->buffer[self->wpos].pkt;
        struct sockaddr_storage *saddr = &self->buffer[self->wpos].addr;
        size_t sz;
        if ((sz = intransport_recv (self->trans, daddr, 4096, saddr))) {
            self->buffer[self->wpos].sz = sz;
            self->wpos++;
            if (self->wpos > self->sz) self->wpos -= self->sz;
            int backlog = (self->wpos - self->rpos);
            if (backlog<0) backlog += self->sz;
            if (backlog > 255) {
                errcnt++;
                if (! (errcnt & 63)) {
                    log_error ("UDP backlog: %i", backlog);
                }
            }
        }
    }
}

/** Create a packetqueue thread.
  * \param qcount Size of the queue (in packets).
  * \param producer An intransport to receive packets on.
  * \return Thread object.
  */
thread *packetqueue_create (size_t qcount, intransport *producer) {
    packetqueue *self = (packetqueue *) malloc (sizeof (packetqueue));
    self->super.run = packetqueue_run;
    self->super.isrunning = 0;
    self->trans = producer;
    self->buffer = (pktbuf *) malloc (qcount * sizeof (pktbuf));
    self->sz = qcount;
    self->rpos = self->wpos = 0;
    pthread_attr_init (&self->super.tattr);
    pthread_mutexattr_init (&self->mattr);
    pthread_mutex_init (&self->mutex, &self->mattr);
    pthread_cond_init (&self->cond, NULL);
    pthread_create (&self->super.thread, &self->super.tattr, thread_spawn, self);
    return (thread *) self;
}
