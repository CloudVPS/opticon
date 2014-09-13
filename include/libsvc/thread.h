#ifndef _THREAD_H
#define _THREAD_H 1

#include <pthread.h>

/* =============================== TYPES =============================== */

struct thread_s; /* forward declaration */

typedef void (*run_f)(struct thread_s *);

/** Base representation of a thread */
typedef struct thread_s {
    run_f            run; /**< The run function */
    pthread_attr_t   tattr; /**< Pthread storage */
    pthread_t        thread; /**< Pthread storage */
    int              isrunning; /**< 1 if thread is spawned */
} thread;

/* ============================= FUNCTIONS ============================= */

void        *thread_spawn (void *);
thread      *thread_create (run_f);
void         thread_init (thread *, run_f);

#endif
