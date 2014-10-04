#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>
#include <libopticon/util.h>
#include <libopticon/hash.h>
#include "tokencache.h"

tokencache TOKENCACHE;

/** Reset a tcache_node to unset.
  * \param self The node to clear
  */
void tcache_node_clear (tcache_node *self, int dofree) {
    if (dofree) {
        if (self->tenantlist) free (self->tenantlist);
    }
    self->token[0] = 0;
    self->hashcode = 0;
    self->lastref = 0;
    self->ctime = 0;
    self->tenantlist = NULL;
    self->tenantcount = 0;
    self->userlevel = AUTH_GUEST;
}

tcache_node *tcache_node_copy (tcache_node *src) {
    tcache_node *dst = (tcache_node *) malloc (sizeof (tcache_node));
    if (! dst) return dst;
    
    memcpy (dst, src, sizeof (tcache_node));
    if (dst->tenantcount) {
        size_t sz = dst->tenantcount * sizeof(uuid);
        dst->tenantlist = (uuid *) malloc (sz);
        memcpy (dst->tenantlist, src->tenantlist, sz);
    }
    return dst;
}

void tcache_node_free (tcache_node *self) {
    tcache_node_clear (self, 1);
    free (self);
}

/** Initialize the global TOKENCACHE */
void tokencache_init (void) {
    int i;
    
    for (i=0; i<256; ++i) {
        tcache_node_clear (&TOKENCACHE.nodes[i], 0);
    }
    for (i=0; i<16; ++i) {
        tcache_node_clear (&TOKENCACHE.invalids[i], 0);
    }
    
    pthread_rwlock_init (&TOKENCACHE.lock, NULL);
    TOKENCACHE.count = 0;
    TOKENCACHE.last_expire = time (NULL);
}

/** Find a cached positive or negative result for a specific
  * authentication token.
  * \param token The token to look up
  * \return The cached result (negative results will have a userlevel
  *         of AUTH_GUEST and a tenantid of nil) or NULL if no cached
  *         data was found.
  */
tcache_node *tokencache_lookup (const char *token) {
    uint32_t hash = hash_token (token);
    tcache_node *crsr;
    tcache_node *res;
    time_t tnow = time (NULL);
    
    pthread_rwlock_rdlock (&TOKENCACHE.lock);
    
    int i;
    for (i=0; i<16; ++i) {
        crsr = &TOKENCACHE.invalids[i];
        if (crsr->hashcode == hash) {
            if (strcmp (token, crsr->token) != 0) continue;
            if ((crsr->ctime + TOKEN_TIMEOUT_INVALID) > tnow) {
                crsr->lastref = tnow;
                res = tcache_node_copy (crsr);
                pthread_rwlock_unlock (&TOKENCACHE.lock);
                tokencache_expire();
                return res;
            }
            break;
        }
    }
    for (i=0; i<TOKENCACHE.count; ++i) {
        crsr = &TOKENCACHE.nodes[i];
        if (crsr->hashcode == hash) {
            if (strcmp (token, crsr->token) != 0) continue;
            if ((crsr->ctime + TOKEN_TIMEOUT_VALID) > tnow) {
                crsr->lastref = tnow;
                res = tcache_node_copy (crsr);
                pthread_rwlock_unlock (&TOKENCACHE.lock);
                tokencache_expire();
                return res;
            }
            break;
        }
    }
    
    pthread_rwlock_unlock (&TOKENCACHE.lock);
    return NULL;
}

/** Clear out expired tokencache nodes */
void tokencache_expire (void) {
    time_t tnow = time (NULL);
    if (tnow - TOKENCACHE.last_expire < 60) return;
    
    pthread_rwlock_wrlock (&TOKENCACHE.lock);
    int i;
    tcache_node *crsr;
    
    for (i=0; i<16; ++i) {
        crsr = &TOKENCACHE.invalids[i];
        if ((crsr->ctime + TOKEN_TIMEOUT_INVALID) <= tnow) {
            tcache_node_clear (crsr, 1);
        }
    }
    for (i=0; i<TOKENCACHE.count; ++i) {
        crsr = &TOKENCACHE.nodes[i];
        if ((crsr->ctime + TOKEN_TIMEOUT_VALID) <= tnow) {
            tcache_node_clear (crsr, 1);
        }
    }
    
    TOKENCACHE.last_expire = tnow;
    
    pthread_rwlock_unlock (&TOKENCACHE.lock);
}

/** Store an invalid token in the cache to save roundtrips on wrong
  * sessions that keep knocking.
  * \param token The token to cache as badnotgood.
  */
void tokencache_store_invalid (const char *token) {
    int i;
    tcache_node *crsr;
    tcache_node *into = NULL;
    time_t tnow = time (NULL); 
    time_t oldest = tnow;
    
    pthread_rwlock_wrlock (&TOKENCACHE.lock);
    
    for (i=0; i<16; ++i) {
        crsr = &TOKENCACHE.invalids[i];
        if ((crsr->hashcode == 0) || 
            (crsr->ctime + TOKEN_TIMEOUT_INVALID) <= tnow) {
            into = crsr;
            break;
        }
        if (crsr->lastref < oldest) {
            oldest = crsr->lastref;
            into = crsr;
        }
    }
    
    if (! into) into = &TOKENCACHE.invalids[rand()&15];
    into->hashcode = hash_token (token);
    strncpy (into->token, token, 1023);
    into->token[1023] = 0;
    into->userlevel = AUTH_GUEST;
    if (into->tenantlist) free (into->tenantlist);
    into->tenantlist = NULL;
    into->tenantcount = 0;
    into->ctime = into->lastref = tnow;

    /* Invalidate any open entries in the positive cache */
    for (i=0; i<TOKENCACHE.count; ++i) {
        crsr = &TOKENCACHE.nodes[i];
        if (crsr->hashcode == into->hashcode) {
            tcache_node_clear (&crsr, 1);
        }
    }
    
    
    pthread_rwlock_unlock (&TOKENCACHE.lock);
}

void __b__ (void) {};

/** Store a valid token in the cache.
  * \param token The string token
  * \param tenantid The resolved tenantid
  * \param userlevel The resolved access level
  * \param name The resolved tenant name
  */
void tokencache_store_valid (const char *token, uuid *tenantlist,
                             int tenantcount, auth_level userlevel) {
    int i;
    tcache_node *crsr;
    tcache_node *into = NULL;
    time_t tnow = time (NULL); 
    time_t oldest = tnow;
    
    pthread_rwlock_wrlock (&TOKENCACHE.lock);
    
    if (TOKENCACHE.count < 256) {
        into = &TOKENCACHE.nodes[TOKENCACHE.count];
        TOKENCACHE.count++;
    }
    else {
        for (i=0; i<256; ++i) {
            crsr = &TOKENCACHE.nodes[i];
            if ((crsr->hashcode == 0) || 
                (crsr->ctime + TOKEN_TIMEOUT_VALID) <= tnow) {
                into = crsr;
                break;
            }
            if (crsr->lastref < oldest) {
                oldest = crsr->lastref;
                into = crsr;
            }
        }
        if (! into) into = &TOKENCACHE.nodes[rand()&255];
    }
    
    into->hashcode = hash_token (token);
    strncpy (into->token, token, 1023);
    into->token[1023] = 0;
    into->userlevel = userlevel;
    
    if (into->tenantlist) free (into->tenantlist);
    into->tenantcount = tenantcount;
    into->tenantlist = (uuid *) malloc (tenantcount * sizeof (uuid));
    memcpy (into->tenantlist, tenantlist, tenantcount * sizeof (uuid));
    into->ctime = into->lastref = tnow;
    
    pthread_rwlock_unlock (&TOKENCACHE.lock);
    __b__();
}
