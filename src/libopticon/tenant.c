#include <libopticon/host.h>
#include <libopticon/tenant.h>
#include <libopticon/util.h>
#include <string.h>

tenantlist TENANTS;

/** Initialize the tenant list */
void tenant_init (void) {
    TENANTS.first = TENANTS.last = NULL;
    pthread_rwlock_init (&TENANTS.lock, NULL);
}

/** Allocate a tenant object */
tenant *tenant_alloc (void) {
    tenant *res = (tenant *) malloc (sizeof(tenant));
    res->first = res->last = NULL;
    res->prev = res->next = NULL;
    memset (&res->key, 0, sizeof (aeskey));
    watchlist_init (&res->watch);
    summaryinfo_init (&res->summ);
    notifylist_init (&res->notify);
    pthread_rwlock_init (&res->lock, NULL);
    return res;
}

/** Delete a tenant. This tenant is assumed to have been
  * previously obtained through, e.g., tenant_find(), and
  * will have been properly locked. This function will first
  * put a write lock on neighbouring nodes, so we can safely
  * manipulate their next/prev pointers.
  */
void tenant_delete (tenant *t) {
    if (! t) return;
    pthread_rwlock_wrlock (&TENANTS.lock);
    
    if (t->prev) {
        pthread_rwlock_wrlock (&t->prev->lock);
        t->prev->next = t->next;
    }
    else TENANTS.first = t->next;
    
    if (t->next) {
        pthread_rwlock_wrlock (&t->next->lock);
        t->next->prev = t->prev;
    }
    else TENANTS.last = t->prev;
    
    if (t->prev) pthread_rwlock_unlock (&t->prev->lock);
    if (t->next) pthread_rwlock_unlock (&t->next->lock);
    pthread_rwlock_unlock (&TENANTS.lock);
    pthread_rwlock_unlock (&t->lock);
    
    host *h = t->first;
    host *nh;
    while (h) {
        nh = h->next;
        host_delete (h);
        h = nh;
    }
    watchlist_clear (&t->watch);
    summaryinfo_clear (&t->summ);
    notifylist_clear (&t->notify);
    pthread_rwlock_destroy (&t->lock);
    free (t);
}

/** Find a tenant in the list by id, or create one if it doesn't
    exist yet. */
tenant *tenant_find (uuid tenantid, tenantlock lockt) {
    pthread_rwlock_wrlock (&TENANTS.lock);
    tenant *nt;
    tenant *t = TENANTS.first;
    if (! t) {
        nt = tenant_alloc();
        nt->uuid = tenantid;
        TENANTS.first = TENANTS.last = nt;
        if (lockt == TENANT_LOCK_READ) {
            pthread_rwlock_rdlock (&nt->lock);
        }
        else {
            pthread_rwlock_wrlock (&nt->lock);
        }
        pthread_rwlock_unlock (&TENANTS.lock);
        return nt;
    }
    while (t) {
        if (uuidcmp (t->uuid, tenantid)) break;
        if (t->next) {
            t = t->next;
        }
        else {
            nt = tenant_alloc();
            nt->uuid = tenantid;
            t->next = nt;
            nt->prev = t;
            TENANTS.last = nt;
            t = nt;
            break;
        }
    }
    if (t) {
        if (lockt == TENANT_LOCK_READ) {
            pthread_rwlock_rdlock (&t->lock);
        }
        else {
            pthread_rwlock_wrlock (&t->lock);
        }
    }
    pthread_rwlock_unlock (&TENANTS.lock);
    return t;
}

/** Get a pointer to the first tenant. It will be locked with the
  * required lock type.
  */
tenant *tenant_first (tenantlock lockt) {
    tenant *res = NULL;
    pthread_rwlock_rdlock (&TENANTS.lock);
    if (TENANTS.first) {
        if (lockt == TENANT_LOCK_READ) {
            pthread_rwlock_rdlock (&TENANTS.first->lock);
        }
        else {
            pthread_rwlock_wrlock (&TENANTS.first->lock);
        }
        res = TENANTS.first;
    }
    pthread_rwlock_unlock (&TENANTS.lock);
    return res;
}

tenant *tenant_next (tenant *t, tenantlock lockt) {
    if (! t) return NULL;
    pthread_rwlock_rdlock (&TENANTS.lock);
    tenant *next = t->next;
    if (next) {
        if (lockt == TENANT_LOCK_READ) {
            pthread_rwlock_rdlock (&next->lock);
        }
        else {
            pthread_rwlock_wrlock (&next->lock);
        }
    }
    pthread_rwlock_unlock (&t->lock);
    pthread_rwlock_unlock (&TENANTS.lock);
    return next;
}

void tenant_done (tenant *t) {
    if (t) pthread_rwlock_unlock (&t->lock);
}

/** Create a new tenant */
tenant *tenant_create (uuid tenantid, aeskey key) {
    tenant *t = tenant_find (tenantid, TENANT_LOCK_WRITE);
    t->key = key;
    return t;
}

/** Set notification status for a host */
void tenant_set_notification (tenant *self, bool isproblem,
                              const char *status, uuid hostid) {
    if (! self) return;
    notification *n = notifylist_find (&self->notify, hostid);
    if (n) {
        strncpy (n->status, status, 15);
        n->status[15] = 0;

        /* Don't do anything else if we're still in the same amount of
           trouble */
        if (n->isproblem == isproblem) return;

        /* is it a problem that went away before notification? */
        if (n->isproblem && (! n->notified)) {
            notifylist_remove (&self->notify, n);
            notification_delete (n);
            return;
        }
        n->lastchange = time (NULL);
        n->notified = false;
    }
    else {
        /* Ignore non-problems that weren't problems */
        if (isproblem) {
            n = notification_create();
            strncpy (n->status, status, 15);
            n->status[15] = 0;
            n->notified = false;
            n->isproblem = isproblem;
            n->lastchange = time (NULL);
            n->hostid = hostid;
            notifylist_link (&self->notify, n);
        }
    }
}

/** Check and handle outstanding notifications for a tenant */
var *tenant_check_notification (tenant *self) {
    if (notifylist_check_actionable (&self->notify)) {
        var *env = var_alloc();
        var *nenv = var_get_dict_forkey (env, "issues");
        notification *n = notifylist_find_overdue (&self->notify, NULL);
        while (n) {
            char uuidstr[40];
            uuid2str (n->hostid, uuidstr);
            var *v = var_get_dict_forkey (nenv, uuidstr);
            var_set_str_forkey (v, "status", n->status);
            n->notified = true;
            n = notifylist_find_overdue (&self->notify, n);
        }
        
        return env;
    }
    return NULL;
}
