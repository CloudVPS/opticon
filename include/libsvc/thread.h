#ifndef _THREAD_H
#define _THREAD_H 1

#include <pthread.h>

/* =============================== TYPES =============================== */

struct thread_s; /* forward declaration */

typedef void (*run_f)(struct thread_s *);
typedef void (*cancel_f)(struct thread_s *);

typedef struct conditional_s {
    pthread_mutexattr_t  mattr; /**< Pthread overhead for the conditional */
    pthread_mutex_t      mutex; /**< Pthread overhead for the conditional */
    pthread_cond_t       cond; /**< 'Packet received' conditional */
    int                  queue; /**< Queued up conditions */
} conditional;

/** Base representation of a thread */
typedef struct thread_s {
    run_f            run; /**< The run function */
    cancel_f         cancel; /**< The cancel function */
    pthread_attr_t   tattr; /**< Pthread storage */
    pthread_t        thread; /**< Pthread storage */
    int              isrunning; /**< 1 if thread is spawned */
    conditional     *cshutdown; /**< Conditional to poll for thread shutdown */
} thread;

/* ============================= FUNCTIONS ============================= */

void        *thread_spawn (void *);
void         thread_init (thread *, run_f, cancel_f);
thread      *thread_create (run_f, cancel_f);
void         thread_free (thread *);
void         thread_cancel (thread *);

conditional *conditional_create (void);
void         conditional_free (conditional *);
void         conditional_init (conditional *);
void         conditional_signal (conditional *);
void         conditional_wait (conditional *);
void         conditional_wait_fresh (conditional *);

#endif
