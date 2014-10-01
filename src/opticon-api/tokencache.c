#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>
#include <libopticon/util.h>
#include "tokenccache.h"

static uint32_t IHASH = 0;

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

void tcache_node_clear (tcache_node *self) {
    self->token[0] = 0;
    self->hashcode = 0;
    self->lastref = 0;
    self->tenantid = uuidnil();
    self->userlevel = AUTH_GUEST;
    self->name[0] = 0;
}

void tokencache_init (void) {
    int i;
    
    for (i=0; i<256; ++i) {
        tcache_node_clear (&TOKENCACHE.nodes[i]);
    }
    for (i=0; i<16; ++i) {
        tcache_node_clear (&TOKENCACHE.invalids[i]);
    }
    
    pthread_rwlock_init (&TOKENCACHE.lock, NULL);
    TOKENCACHE.count = 0;
}

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
                return res;
            }
            break;
        }
    }
    
    pthread_rwlock_unlock (&TOKENCACHE.lock);
    return NULL;
}
