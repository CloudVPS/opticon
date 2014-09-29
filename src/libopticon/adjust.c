#include <libopticon/datatypes.h>
#include <libopticon/util.h>
#include <libopticon/log.h>
#include <unistd.h>
#include <stdlib.h>

/** Initialize memory structure. Since it's a list header embedded
  * inside other structures, its allocation is considered Not Our
  * Problem™.
  * \param self The adjustlist to initialize.
  */
void adjustlist_init (adjustlist *self) {
    self->first = self->last = NULL;
}

/** Clear up an adjustlist, deallocating all watchadjust members.
  * \param self The adjustlist to clear.
  */
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

/** Look up a watchadjust by its meterid.
  * \param self The adjustlist
  * \param id The id to look up
  * \return The matching watchadjust, or NULL.
  */
watchadjust *adjustlist_find (adjustlist *self, meterid_t id) {
    watchadjust *w;
    w = self->first;
    while (w) {
        if ((w->id & MMASK_NAME) == (id & MMASK_NAME)) return w;
        w = w->next;
    }
    return NULL;
}

/** Look up or create a watchadjust by its meterid.
  * \param self The adjustlist
  * \param id The id to look up
  * \return The new or existing watchadjust.
  */
watchadjust *adjustlist_get (adjustlist *self, meterid_t id) {
    watchadjust *w = adjustlist_find (self, id);
    if (w) return w;
    
    w = (watchadjust *) calloc (1, sizeof (watchadjust));
    w->type = WATCHADJUST_NONE;
    w->id = (id & MMASK_NAME);
    
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
