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
        if ((sz = intransport_recv (self->trans, daddr, 2048, saddr))) {
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
            pthread_mutex_lock (&self->mutex);
            pthread_cond_signal (&self->cond);
            pthread_mutex_unlock (&self->mutex);
        }
    }
}

/** Wait for a new packet, or pick one out of the queue.
  * \param t The packetqueue thread
  * \return The packetbuffer with received data.
  */
pktbuf *packetqueue_waitpkt (thread *t) {
    packetqueue *self = (packetqueue *) t;
    while (self->rpos == self->wpos) {
        pthread_mutex_lock (&self->mutex);
        pthread_cond_wait (&self->cond, &self->mutex);
        pthread_mutex_unlock (&self->mutex);
    }
    pktbuf *res = self->buffer + self->rpos;
    self->rpos++;
    if (self->rpos >= self->sz) self->rpos -= self->sz;
    return res;
}

/** Create a packetqueue thread.
  * \param qcount Size of the queue (in packets).
  * \param producer An intransport to receive packets on.
  * \return Thread object.
  */
thread *packetqueue_create (size_t qcount, intransport *producer) {
    packetqueue *self = (packetqueue *) malloc (sizeof (packetqueue));
    self->trans = producer;
    self->buffer = (pktbuf *) malloc (qcount * sizeof (pktbuf));
    self->sz = qcount;
    self->rpos = self->wpos = 0;
    thread_init ((thread *) self, packetqueue_run);
    return (thread *) self;
}