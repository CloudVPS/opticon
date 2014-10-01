#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>
#include <libopticon/util.h>
#include "tokencache.h"

static uint32_t IHASH = 0;
tokencache TOKENCACHE;

/** Generic hash-function nicked from grace.
  * \param str The string to hash
  * \return A 32 bit hashed guaranteed not 0.
  */
uint32_t hash_token (const char *str) {
    uint32_t hash = 0;
    uint32_t i    = 0;
    uint32_t s    = 0;
    int sdf;
    
    /* Set up a random intial hash once */
    if (! IHASH)
    {
        sdf = open ("/dev/urandom", O_RDONLY);
        if (sdf<0) sdf = open ("/dev/random", O_RDONLY);
        if (sdf>=0)
        {
            (void) read (sdf, &IHASH, sizeof(IHASH));
            close (sdf);
        }
        if (! IHASH) IHASH = 5138; /* fair dice roll */
    }
    
    hash = IHASH;
    
    const unsigned char* ustr = (const unsigned char*)str;

    for (i = 0; ustr[i]; i++) {
        hash = ((hash << 5) + hash) + (ustr[i] & 0xDF);
    }

    s = hash;
    
    switch (i) { /* note: fallthrough for each case */
        default:
        case 6: hash += (s % 5779) * (ustr[6] & 0xDF);
        case 5: hash += (s % 4643) * (ustr[5] & 0xDF); hash ^= hash << 7;
        case 4: hash += (s %  271) * (ustr[4] & 0xDF);
        case 3: hash += (s % 1607) * (ustr[3] & 0xDF); hash ^= hash << 15;
        case 2: hash += (s % 7649) * (ustr[2] & 0xDF);
        case 1: hash += (s % 6101) * (ustr[1] & 0xDF); hash ^= hash << 25;
        case 0: hash += (s % 1217) * (ustr[0] & 0xDF);
    }
    
    /* zero is inconvenient */
    if (hash == 0) hash = 154004;
    return hash;
}

/** Reset a tcache_node to unset.
  * \param self The node to clear
  */
void tcache_node_clear (tcache_node *self, int dofree) {
    if (dofree) {
        if (self->tenantlist) free (self->tenantlist);
        if (self->tenantnames) var_free (self->tenantnames);
    }
    self->token[0] = 0;
    self->hashcode = 0;
    self->lastref = 0;
    self->ctime = 0;
    self->tenantlist = NULL;
    self->tenantcount = 0;
    self->userlevel = AUTH_GUEST;
    self->tenantnames = NULL;
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
                res = (tcache_node *) malloc (sizeof (tcache_node));
                memcpy (res, crsr, sizeof (tcache_node));
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
                res = (tcache_node *) malloc (sizeof (tcache_node));
                memcpy (res, crsr, sizeof (tcache_node));
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
    if (into->tenantnames) var_free (into->tenantnames);
    into->tenantlist = NULL;
    into->tenantcount = 0;
    into->tenantnames = NULL;
    into->ctime = into->lastref = tnow;
    
    pthread_rwlock_unlock (&TOKENCACHE.lock);
}

/** Store a valid token in the cache.
  * \param token The string token
  * \param tenantid The resolved tenantid
  * \param userlevel The resolved access level
  * \param name The resolved tenant name
  */
void tokencache_store_valid (const char *token, uuid *tenantlist,
                             int tenantcount,
                             auth_level userlevel, var *names) {
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
        for (i=0; i<16; ++i) {
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
    
    if (! into->tenantnames) into->tenantnames = var_alloc();
    var_copy (into->tenantnames, names);
    into->ctime = into->lastref = tnow;
    
    pthread_rwlock_unlock (&TOKENCACHE.lock);
}
