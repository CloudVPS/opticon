notification *notification_create (void) {
    notification *res = (notification *) malloc (sizeof (notification));
    res->next = res->prev = NULL;
    res->problems[0] = 0;
    res->lastchange = time (NULL);
}

void notification_delete (notification *self) {
    free (self);
}

void notifylist_init (notifylist *self) {
    self->first = self->last = NULL;
}

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

notification *notifylist_find (notifylist *self, uuid hostid) {
    notification *n = self->first;
    while (n) {
        if (uuidcmp (hostid, n->hostid)) return n;
        n = n->next;
    }
    return n;
}

bool notifylist_check_actionable (notifylist *self) {
    notification *n = self->first;
    time_t tnow = time (NULL);
    while (n) {
        if (tnow - n->lastchange > 290) return true;
        n = n->next;
    }
    return false;
}

notification *notifylist_find_overdue (notifylist *self, notification *n) {
    if (! n) n = self->first;
    if (! n) return NULL;
    n = n->next;
    
    time_t tnow = time (NULL);
    
    while (n) {
        if (n->lastchange - tnow > 120) return n;
        n = n->next;
    }
    
    return NULL;
}
