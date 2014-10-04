#include <libopticon/datatypes.h>
#include <libopticon/util.h>
#include <libopticon/defaults.h>
#include <assert.h>

/** Allocate and initialize a host structure.
  * \return Pointer to the initialized struct. Caller is responsible for
  *         freeing allocated memory.
  */
host *host_alloc (void) {
    host *res = (host *) malloc (sizeof (host));
    res->prev = NULL;
    res->next = NULL;
    res->first = NULL;
    res->last = NULL;
    res->tenant = NULL;
    res->status = 0;
    res->lastserial = 0;
    res->badness = 0.0;
    res->lastmetasync = 0;
    adjustlist_init (&res->adjust);
    pthread_rwlock_init (&res->lock, NULL);
    return res;
}

/** Find or create a host structure for a specified tenant. 
  * \param tenantid The UUID for the tenant.
  * \param hostid The UUID for the host.
  */
host *host_find (uuid tenantid, uuid hostid) {
    host *h = NULL;
    host *nh = NULL;
    tenant *t = tenant_find (tenantid);
    if (! t) return NULL;
    
    h = t->first;
    if (! h) {
        h = host_alloc();
        h->uuid = hostid;
        h->tenant = t;
        t->first = t->last = h;
        return h;
    }
    
    while (h) {
        if (uuidcmp (h->uuid, hostid)) return h;
        if (h->next) h = h->next;
        else {
            nh = host_alloc();
            nh->uuid = hostid;
            nh->tenant = t;
            h->next = nh;
            nh->prev = h;
            t->last = nh;
            return nh;
        }
    }
    
    return NULL;
}

/** Deallocate a host and its associated meters. Remove it from the
  * tenant, if it is associated with one. 
  * \param h The host to delete
  */
void host_delete (host *h) {
    meter *m, *nm;
    if (h->tenant) {
        if (h->prev) {
            h->prev->next = h->next;
        }
        else h->tenant->first = h->next;
        if (h->next) {
            h->next->prev = h->prev;
        }
        else h->tenant->last = h->prev;
    }
    m = h->first;
    while (m) {
        nm = m->next;
        if (m->d.any) free (m->d.any);
        free (m);
        m = nm;
    }
    pthread_rwlock_destroy (&h->lock);
    adjustlist_clear (&h->adjust);
    free (h);
}

/** Allocate and initialize a meter structure. */
meter *meter_alloc (void) {
    meter *res = (meter *) malloc (sizeof (meter));
    res->next = NULL;
    res->prev = NULL;
    res->count = -1;
    res->d.u64 = NULL;
    res->host = NULL;
    return res;
}

/** Figure out if a meter currently exists for a host.
  * \param host The host object
  * \param id The id of the meter
  * \return 1 on success, 0 on failure
  */
int host_has_meter (host *h, meterid_t id) {
    meterid_t rid = (id & (MMASK_TYPE | MMASK_NAME));
    meter *m = h->first;
    int res = 0;
    while (m) {
        if (m->id == rid) {
            res = 1;
            break;
        }
        m = m->next;
    }
    return res;
}

/** Find a meter only by its name, regardless of type */
meter *host_find_meter_name (host *h, meterid_t id) {
    meterid_t rid = (id & MMASK_NAME);
    meter *m = h->first;
    while (m) {
        if ((m->id & MMASK_NAME) == rid) return m;
        m = m->next;
    }
    return NULL;
}

/** Get (or create) a specific meter for a host.
  * \param host The host structure.
  * \param id The meterid (label and type).
  * \return Pointer to the meter structure, which will be
  *         linked into the host's meterlist.
  */
meter *host_get_meter (host *h, meterid_t id) {
    meterid_t rid = (id & (MMASK_TYPE | MMASK_NAME));
    meter *m = h->first;
    meter *nm = NULL;
    if (! m) {
        nm = meter_alloc();
        h->first = h->last = nm;
        nm->id = rid;
        nm->host = h;
        return nm;
    }
    while (m) {
        if (m->id == rid) return m;
        if (m->next) m = m->next;
        else {
            nm = meter_alloc();
            m->next = nm;
            nm->prev = m;
            h->last = nm;
            nm->id = rid;
            nm->host = h;
            return nm;
        }
    }
    return NULL;
}

meter *host_find_prefix (host *h, meterid_t prefix, meter *prev) {
    meter *crsr = prev ? prev : h->first;
    meterid_t mask = id2mask (prefix);
    crsr = crsr->next;
    while (crsr) {
        char meter[16];
        id2str (crsr->id &mask, meter);
        if ((crsr->id & mask) == (prefix & MMASK_NAME)) {
            if (idisprefix (prefix, crsr->id, mask)) return crsr;
        }
        crsr = crsr->next;
    }
    return NULL;
}

/** Get a specific indexed integer value out of a meter */
uint64_t meter_get_uint (meter *m, unsigned int pos) {
    uint64_t res = 0ULL;
    
    if (m->count >= SZ_EMPTY_VAL) return res;

    if (pos < (m->count?m->count:1) &&
        (m->id & MMASK_TYPE) == MTYPE_INT) {
        res = m->d.u64[pos];
    }

    return res;
}

/** Get a specific indexed fractional value out of a meter */
double meter_get_frac (meter *m, unsigned int pos) {
    double res = 0.0;
    
    if (m->count >= SZ_EMPTY_VAL) return res;

    if (pos < (m->count?m->count:1) &&
        (m->id & MMASK_TYPE) == MTYPE_FRAC) {
        res = m->d.frac[pos];
    }

    return res;
}

/** Get a string value out of a meter */
fstring meter_get_str (meter *m, unsigned int pos) {
    fstring res = {""};
    
    if (m->count >= SZ_EMPTY_VAL) return res;
    if (m->d.any == NULL) return res;

    if (pos < (m->count?m->count:1) &&
        (m->id & MMASK_TYPE) == MTYPE_STR) {
        strcpy (res.str, m->d.str[pos].str);
    }

    return res;
}

/** Remove a meter from a host list and deallocate it */
void host_delete_meter (host *h, meter *m) {
    if (m->prev) {
        m->prev->next = m->next;
    }
    else {
        h->first = m->next;
    }
    if (m->next) {
        m->next->prev = m->prev;
    }
    else {
        h->last = m->prev;
    }
    meter_set_empty (m);
    free (m);
}

/** Mark the beginning of a new update cycle. Saves the meters
  * from constantly asking the kernel for the current time.
  */
void host_begin_update (host *h, time_t t) {
    h->lastmodified = t;
}

/** End an update round. Reaps any dangling meters that have been
    inactive for more than five minutes. */
void host_end_update (host *h) {
    time_t last = h->lastmodified;
    meter *m = h->first;
    meter *nm;
    while (m) {
        nm = m->next;
        if (m->lastmodified < last) {
            if ((m->lastmodified - last) > default_meter_timeout) {
                host_delete_meter (h, m);
            }
        }
        m = nm;
    }
}

/** Fill up a meter with integer values */
meter *host_set_meter_uint (host *h, meterid_t id,
                            unsigned int cnt,
                            uint64_t *data) {
    
    unsigned int count = cnt ? cnt : 1;
    meter *m = host_get_meter (h, id);
    m->count = cnt;
    m->lastmodified = h->lastmodified;
    
    if (m->d.any != NULL) {
        free (m->d.any);
        m->d.any = NULL;
    }
    
    m->d.u64 = (uint64_t *) malloc (count * sizeof (uint64_t));
    memcpy (m->d.u64, data, count * sizeof (uint64_t));
    return m;
}

/** Fill up a meter with string values */
meter *host_set_meter_str (host *h, meterid_t id,
                           unsigned int cnt,
                           const fstring *str) {
    unsigned int count = cnt ? cnt : 1;
    meter *m = host_get_meter (h, id);
    m->count = cnt;
    m->lastmodified = h->lastmodified;
    if (m->d.any != NULL) {
        free (m->d.any);
        m->d.any = NULL;
    }
    m->d.str = (fstring *) malloc (count * sizeof (fstring));
    memcpy (m->d.str, str, count * sizeof (fstring));
    return m;   
}

void meter_set_empty (meter *m) {
    meter_setcount (m, SZ_EMPTY_VAL);
}

void meter_set_empty_array (meter *m) {
    meter_setcount (m, SZ_EMPTY_ARRAY);
}

/** Initialize a meter to a specific set size */
void meter_setcount (meter *m, unsigned int count) {
    int cnt = count ? count : 1;
    assert (cnt <= SZ_EMPTY_ARRAY);

    /* Empty value/array shortcut: deallocate and be done */    
    if (cnt >= SZ_EMPTY_VAL) {
        m->count = cnt;
        m->lastmodified = m->host->lastmodified;
        if (m->d.any) {
            free (m->d.any);
            m->d.any = NULL;
        }
        return;
    }
    
    m->count = count;
    m->lastmodified = m->host->lastmodified;
    
    void *oldarray = m->d.any;
    
    switch (m->id & MMASK_TYPE) {
        case MTYPE_INT:
            m->d.u64 = (uint64_t *) calloc (cnt, sizeof (uint64_t));
            break;
        
        case MTYPE_FRAC:
            m->d.frac = (double *) calloc (cnt, sizeof (double));
            break;
        
        case MTYPE_STR:
            m->d.str = (fstring *) calloc (cnt, sizeof (fstring));
            break;
        
        default:
            m->d.any = NULL;
            m->count = -1;
            break;
    }
    
    if (oldarray) free (oldarray);
}

/** Setter for a specific integer value inside a meter array */
void meter_set_uint (meter *m, unsigned int pos, uint64_t val) {
    if (m->count >= SZ_EMPTY_VAL) return;
    if ((m->id & MMASK_TYPE) == MTYPE_INT &&
        pos < (m->count ? m->count : 1)) {
        m->lastmodified = m->host->lastmodified;
        m->d.u64[pos] = val;
    }
}

/** Setter for a specific fractional value inside a meter array */
void meter_set_frac (meter *m, unsigned int pos, double val) {
    if (m->count >= SZ_EMPTY_VAL) return;
    if ((m->id & MMASK_TYPE) == MTYPE_FRAC &&
        pos < (m->count ? m->count : 1)) {
        m->lastmodified = m->host->lastmodified;
        m->d.frac[pos] = val;
    }
}

/** Setter for a specific string value inside a meter array */
void meter_set_str (meter *m, unsigned int pos, const char *val) {
    if (m->count >= SZ_EMPTY_VAL) return;
    if ((m->id & MMASK_TYPE) == MTYPE_STR &&
        pos < (m->count ? m->count : 1)) {
        strncpy (m->d.str[pos].str, val, 127);
        m->lastmodified = m->host->lastmodified;
        m->d.str[pos].str[127] = '\0';
    }
}

/** Find the next sibling for a meter with a path prefix */
meter *meter_next_sibling (meter *m) {
    uint64_t mask = idhaspath (m->id);
    if (! mask) return NULL;

    meter *crsr = m->next;
    while (crsr) {
        if ((crsr->id & mask) == (m->id & mask)) return crsr;
        crsr = crsr->next; 
    }
    return NULL;
}
