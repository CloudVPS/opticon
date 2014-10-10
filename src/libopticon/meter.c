#include <libopticon/meter.h>
#include <libopticon/util.h>
#include <libopticon/defaults.h>
#include <assert.h>

/** Allocate and initialize a meter structure. */
meter *meter_alloc (void) {
    meter *res = (meter *) malloc (sizeof (meter));
    res->next = NULL;
    res->prev = NULL;
    res->count = -1;
    res->d.u64 = NULL;
    res->host = NULL;
    res->badness = 0.0;
    return res;
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
