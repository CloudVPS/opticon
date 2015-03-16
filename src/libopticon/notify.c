#include <libopticon/notify.h>
#include <unistd.h>
#include <stdlib.h>

/** Allocate and initialize a notification object */
notification *notification_create (void) {
    notification *res = (notification *) malloc (sizeof (notification));
    res->next = res->prev = NULL;
    res->problems[0] = 0;
    res->status[0] = 0;
    res->isproblem = false;
    res->notified = false;
    res->hostid = (uuid){0ULL,0ULL};
    res->lastchange = time (NULL);
    return res;
}

/** Free up memory for a notification object. Should be removed from its
  * list using notifylist_remove() first.
  * \param self The notification object.
  */
void notification_delete (notification *self) {
    free (self);
}

/** Initialize a notifylist object.
  * \param self The notifylist object to initialize.
  */
void notifylist_init (notifylist *self) {
    self->first = self->last = NULL;
}

/** Clean up the linked list */
void notifylist_clear (notifylist *self) {
    notification *n, *nn;
    n = self->first;
    while (n) {
        nn = n->next;
        notification_delete (n);
        n = nn;
    }
    self->first = self->last = NULL;
}

/** Removes a notification object from a notifylist.
  * \param self The list to remove from.
  * \param n The notification object to unlink.
  */
void notifylist_remove (notifylist *self, notification *n) {
    if (n->prev) {
        if (n->next) {
            n->next->prev = n->prev;
            n->prev->next = n->next;
        }
        else {
            n->prev->next = NULL;
            self->last = n->prev;
        }
    }
    else {
        if (n->next) {
            n->next->prev = NULL;
            self->first = n->next;
        }
        else {
            self->first = self->last = NULL;
        }
    }
    
    notification_delete(n);
}

/** Add a notification object to a notifylist.
  * \param self The notifylist to add to.
  * \param n The notification object to link.
  */
void notifylist_link (notifylist *self, notification *n) {
    n->next = NULL;
    if (self->last) {
        n->prev = self->last;
        self->last->next = n;
    }
    else {
        n->prev = NULL;
        self->last = self->first = n;
    }
}

/** Find a specific host in a tenant's notifylist.
  * \param self The notifylist to search.
  * \param hostid The host uuid to search for.
  * \return Pointer to the notification object, or NULL if not found.
  */
notification *notifylist_find (notifylist *self, uuid hostid) {
    notification *n = self->first;
    while (n) {
        if (uuidcmp (hostid, n->hostid)) return n;
        n = n->next;
    }
    return n;
}

/** Find out if there are actionable notifications in a notifylist.
  * The definition of 'actionable' is: At least one notification has
  * been queued for at least 290 seconds.
  * \param self The notifylist to inspect.
  * \return true if there's action afoot.
  */
bool notifylist_check_actionable (notifylist *self) {
    notification *n = self->first;
    time_t tnow = time (NULL);
    while (n) {
        if ((tnow - n->lastchange > 290) && (!n->notified)) return true;
        n = n->next;
    }
    return false;
}

/** Get the first, or next, notification in a notifylist that should be
  * included in an outgoing message. Any notification that has been
  * around for more than 2 minutes is considered in this stage. This
  * function is not supposed to be called before getting a positive result
  * out of notifylist_check_actionable(), so the end result is that
  * a message will be sent for all notifications due for at least 2 minutes,
  * as long as there is one that is due for at least 5 minutes.
  * \param self The notifylist to search.
  * \pram n The last notification object returned, or NULL to start a new
  *         round.
  */
notification *notifylist_find_overdue (notifylist *self, notification *n) {
    if (! n) {
        n = self->first;
    }
    else n = n->next;
    if (! n) return NULL;
    
    time_t tnow = time (NULL);
    
    while (n) {
        if ((tnow - n->lastchange > 120) && (! n->notified)) return n;
        n = n->next;
    }
    
    return NULL;
}
