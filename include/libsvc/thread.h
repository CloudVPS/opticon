#ifndef _THREAD_H
#define _THREAD_H 1

#include <pthread.h>

struct thread_s; /* forward declaration */

typedef void (*run_f)(struct thread_s *);

typedef struct thread_s {
    run_f            run;
    pthread_attr_t   tattr;
    pthread_t        thread;
} thread;

thread *thread_create (run_f);

#endif
