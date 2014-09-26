#include <libopticon/datatypes.h>
#include <unistd.h>
#include <stdlib.h>

void watchlist_init (watchlist *self) {
    self->first = self->last = NULL;
    pthread_mutex_init (&self->mutex, NULL);
}

void watchlist_add (watchlist *self, meterwatch *m) {
    if (self->last) {
        m->prev = self->last;
        self->last->next = m;
        self->last = m;
    }
    else {
        self->first = self->last = m;
    }
}

void watchlist_add_uint (watchlist *self, meterid_t id, watchtype tp,
                         uint64_t val, double bad, watchtrigger trig) {
    meterwatch *w = (meterwatch *) malloc (sizeof (meterwatch));
    w-> next = w->prev = NULL;
    w->id = id;
    w->tp = tp;
    w->trigger = trig;
    w->badness = bad;
    w->dat.u64 = val;
    watchlist_add (self, w);
}

void watchlist_add_frac (watchlist *self, meterid_t id, watchtype tp,
                         double val, double bad, watchtrigger trig) {
    meterwatch *w = (meterwatch *) malloc (sizeof (meterwatch));
    w-> next = w->prev = NULL;
    w->id = id;
    w->tp = tp;
    w->trigger = trig;
    w->badness = bad;
    w->dat.frac = val;
    watchlist_add (self, w);
}

void watchlist_add_str (watchlist *self, meterid_t id, watchtype tp,
                        const char *val, double bad, watchtrigger trig) {
                        
    meterwatch *w = (meterwatch *) malloc (sizeof (meterwatch));
    w-> next = w->prev = NULL;
    w->id = id;
    w->tp = tp;
    w->trigger = trig;
    w->badness = bad;
    strncpy (w->dat.str.str, val, 127);
    w->dat.str.str[127] = 0;
    watchlist_add (self, w);
}

void watchlist_clear (watchlist *self) {
    meterwatch *w;
    meterwatch *nw;
    w = self->first;
    while (w) {
        nw = w->next;
        free (w);
        w = nw;
    }
}
