#include <libopticon/var_parse.h>
#include <libopticon/log.h>
#include "opticon-agent.h"
#include "probes.h"

/** Allocate and initialize a probe object in memory */
probe *probe_alloc (void) {
    probe *res = (probe *) malloc (sizeof (probe));
    conditional_init (&res->pulse);
    res->type = PROBE_NONE;
    res->call = NULL;
    res->prev = res->next = NULL;
    res->vcurrent = res->vold = NULL;
    res->lastpulse = 0;
    res->lastdispatch = time (NULL);
    res->lastreply = res->lastdispatch - 1;
    res->interval = 0;
    return res;
}

/** Implementation of the exec probe type. Reads JSON from the
  * stdout of the callde program. */
var *runprobe_exec (probe *self) {
    char buffer[4096];
    buffer[0] = 0;
    FILE *proc = popen (self->call, "r");
    if (! proc) return NULL;
    while (!feof (proc)) {
        size_t res = fread (buffer, 1, 4096, proc);
        if (res && res < 4096) {
            buffer[res] = 0;
            break;
        }
    }
    pclose (proc);
    var *res = var_alloc();
    if (! var_parse_json (res, buffer)) {
        log_error ("Error parsing output from %s: %s", self->call,
                    parse_error());
        var_free (res);
        return NULL;
    }
    return res;
}

/** Main thread loop for all probes. Waits for a pulse, then gets to
  * work.
  */
void probe_run (thread *t) {
    probe *self = (probe *) t;
    self->vold = self->vcurrent = NULL;
    log_info ("Starting probe %s", self->call);
    
    while (1) {
        var *nvar = self->func (self);
        volatile var *ovar = self->vold;
        if (nvar) {
            self->vold = self->vcurrent;
            self->vcurrent = nvar;
            self->lastreply = time (NULL);
            if (ovar) var_free ((var *)ovar);
        }
        else {
            log_error ("No suitable response from probe");
        }
        conditional_wait_fresh (&self->pulse);
        log_debug ("Probe %s pulse received", self->call);
    }
}

/** Look up a built-in probe by name */
probefunc_f probe_find_builtin (const char *id) {
    int i = 0;
    while (BUILTINS[i].name) {
        if (strcmp (BUILTINS[i].name, id) == 0) {
            return BUILTINS[i].func;
        }
        i++;
    }
    return NULL;
}

/** Add a probe to a list */
int probelist_add (probelist *self, probetype t, const char *call, int iv) {
    probefunc_f func;
    if (t == PROBE_BUILTIN) func = probe_find_builtin (call);
    else func = runprobe_exec;
    if (! func) return 0;
    
    probe *p = probe_alloc();
    p->type = t;
    p->call = strdup (call);
    p->func = func;
    p->interval = iv;
    
    if (self->last) {
        p->prev = self->last;
        self->last->next = p;
        self->last = p;
    }
    else {
        self->first = self->last = p;
    }
    return 1;
}

/** Start all probes in a list */
void probelist_start (probelist *self) {
    probe *p = self->first;
    while (p) {
        thread_init (&p->thr, probe_run, NULL);
        p = p->next;
    }
}

/** Cancels all probes in a list */
void probelist_cancel (probelist *self) {
    probe *p = self->first;
    while (p) {
        thread_cancel (&p->thr);
    }
}
