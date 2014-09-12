#include <libsvc/packetqueue.h>
#include <libsvc/thread.h>
#include <libsvc/transport.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

void packetqueue_run (thread *t) {
    packetqueue *self = (packetqueue *) t;
}

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
