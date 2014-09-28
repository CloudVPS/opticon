#include <libopticon/datatypes.h>
#include <unistd.h>
#include <stdlib.h>

void adjustlist_init (adjustlist *self) {
    self->first = self->last = NULL;
}

void adjustlist_clear (adjustlist *self) { 
    watchadjust *w, *nw;
    w = self->first;
    while (w) {
        nw = w->next;
        free (w);
        w = nw;
    }
    self->first = self->last = NULL;
}

watchadjust *adjustlist_find (adjustlist *self, meterid_t id) {
    watchadjust *w;
    w = self->first;
    while (w) {
        if ((w->id & MMASK_NAME) == (id & MMASK_NAME)) return w;
        w = w->next;
    }
    return NULL;
}

watchadjust *adjustlist_get (adjustlist *self, meterid_t id) {
    watchadjust *w = adjustlist_find (self, id);
    if (w) return w;
    
    w = (watchadjust *) calloc (1, sizeof (watchadjust));
    w->type = WATCHADJUST_NONE;
    
    if (self->last) {
        w->prev = self->last;
        self->last->next = w;
        self->last = w;
    }
    else {
        self->first = self->last = w;
    }
    return w;
}

void watchadjust_set_int (watchadjust *self, watchtrigger tr, 
                          uint64_t val) {
    if (tr >= WATCH_NONE) return;
    self->adjust[tr].data.u64 = val;
}

void watchadjust_set_frac (watchadjust *self, watchtrigger tr,
                           double val) {
    if (tr >= WATCH_NONE) return;
    self->adjust[tr].data.frac = val;
}

void watchadjust_set_str (watchadjust *self, watchtrigger tr,
                          const char *val) {
    if (tr >= WATCH_NONE) return;
    strncpy (self->adjust[tr].data.str.str, val, 127);
    self->adjust[tr].data.str.str[127] = 0;
}
