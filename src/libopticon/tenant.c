#include <libopticon/datatypes.h>
#include <libopticon/util.h>
#include <string.h>

tenantlist TENANTS;

/** Initialize the tenant list */
void tenant_init (void) {
    TENANTS.first = TENANTS.last = NULL;
}

/** Allocate a tenant object */
tenant *tenant_alloc (void) {
    tenant *res = (tenant *) malloc (sizeof(tenant));
    res->first = res->last = NULL;
    res->prev = res->next = NULL;
    memset (&res->key, 0, sizeof (aeskey));
    return res;
}

/** Find a tenant in the list by id, or create one if it doesn't
    exist yet. */
tenant *tenant_find (uuid tenantid) {
    tenant *nt;
    tenant *t = TENANTS.first;
    if (! t) {
        nt = tenant_alloc();
        nt->uuid = tenantid;
        TENANTS.first = TENANTS.last = nt;
        return nt;
    }
    while (t) {
        if (uuidcmp (t->uuid, tenantid)) return t;
        if (t->next) {
            t = t->next;
        }
        else {
            nt = tenant_alloc();
            nt->uuid = tenantid;
            t->next = nt;
            nt->prev = t;
            TENANTS.last = nt;
            return nt;
        }
    }
    
    return NULL;
}

/** Create a new tenant */
tenant *tenant_create (uuid tenantid, aeskey key) {
    tenant *t = tenant_find (tenantid);
    t->key = key;
    return t;
}
