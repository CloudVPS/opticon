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
    pthread_rwlock_init (&res->lock, NULL);
    return res;
}

void tenant_delete (tenant *t) {
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
    pthread_rwlock_unlock (&t->lock);
}

/** Create a new tenant */
tenant *tenant_create (uuid tenantid, aeskey key) {
    tenant *t = tenant_find (tenantid, TENANT_LOCK_WRITE);
    t->key = key;
    return t;
}
