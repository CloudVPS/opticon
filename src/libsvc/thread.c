#include <libsvc/thread.h>
#include <stdlib.h>

/** Call a thread's run function */
void *thread_spawn (void *dt) {
    thread *self = (thread *) dt;
    self->run (self);
    return NULL;
}

/** Create and spawn a thread */
thread *thread_create (run_f run) {
    thread *self = (thread *) malloc (sizeof (thread));
    self->run = run;
    pthread_attr_init (&self->tattr);
    pthread_create (&self->thread, &self->tattr, thread_spawn, self);
    return self;
}
