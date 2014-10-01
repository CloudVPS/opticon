#ifndef _TOKENCACHE_H
#define _TOKENCACHE_H 1

#include <libopticon/uuid.h>
#include <pthread.h>
#include "req_context.h"

/* =============================== TYPES =============================== */

#define TOKEN_TIMEOUT_VALID 3600 /**< Re-verify valid tokens after 1h */
#define TOKEN_TIMEOUT_INVALID 60 /**< Re-try invalid tokens after 1m */

/** Structure representing a cached authentication token and the
  * results of its lookup at the backend
  */
typedef struct tcache_node_s {
    char         token[1024]; /**< String representation of token */
    uint32_t     hashcode; /**< Hash code of same */
    time_t       lastref; /**< Time token was last referenced */
    time_t       ctime; /**< Time cache-entry was created */ 
    uuid        *tenantlist; /**< Tenants bound to the token */
    int          tenantcount; /**< Number of tenants */
    auth_level   userlevel; /**< Userlevel for the token */
    char         name[256]; /**< Tenant name for the token */
} tcache_node;

/** Structure keeping a cache of valid and invalid tokens */
typedef struct tokencache_s {
    tcache_node          nodes[256]; /**< Array of cached valid tokens */
    tcache_node          invalids[16]; /**< Array of cached invalid tokens */
    int                  count; /**< Number of tokens in the valid cache */
    time_t               last_expire; /**< Time of last expiration round */
    pthread_rwlock_t     lock; /**< Thread guard */
} tokencache;

/* ============================== GLOBALS ============================== */

/** Global token cache. The old school singleton. */
extern tokencache TOKENCACHE; 

/* ============================= FUNCTIONS ============================= */

void         tokencache_init (void);
tcache_node *tokencache_lookup (const char *token);
void         tokencache_expire (void);
void         tokencache_store_invalid (const char *token);
void         tokencache_store_valid (const char *token, uuid *tenants,
                                     int numtenants, auth_level userlevel,
                                     const char *name);

#endif
