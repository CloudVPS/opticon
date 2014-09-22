#include "probe.h"

probefunc BUILTINS[] = {
    {"pcpu", runprobe_pcpu},
    {"top", runprobe_top},
    {NULL, NULL}
};

probe *probe_alloc (void) {
    probe *res = (probe *) malloc (sizeof (probe));
    conditional_init (&res->pulse);
    res->type = PROBE_NONE;
    res->call = NULL;
    res->prev = res->next = NULL;
    res->vcurrent = res->vold = NULL;
    res->lastpulse = res->lastreply = 0;
    res->interval = 0;
    return res;
}

void probelist_add (probelist *self, probetype t, const char *call, int iv) {
    probe *p = probe_alloc();
    p->type = t;
    p->call = strdup (call);
    p->interval = iv;
    
    if (self->last) {
        p->prev = self->last;
        self->last->next = p;
        self->last = p;
    }
    else {
        self->first = self->last = p;
    }
}

void probelist_start (probelist *self) {
    probe *p = self->first;
    while (p) {
        if (p->type == PROBE_BUILTIN) {
            run_f tocall = probefunc_find_builtin (p->call);
            if (tocall) {
                thread_init (&p->thr, tocall, NULL);
            }
        }
        else if (p->type == PROBE_EXEC) {
            thread_init (&p->thr, runprobe_exec);
        }
    }
}
