#ifndef _TOKENCACHE_H
#define _TOKENCACHE_H 1

#include <libopticon/uuid.h>
#include <pthread.h>
#include "req_context.h"

#define TOKEN_TIMEOUT_VALID 3600
#define TOKEN_TIMEOUT_INVALID 60

typedef struct tcache_node_s {
    char         token[1024];
    uint32_t     hashcode;
    time_t       lastref;
    uuid         tenantid;
    auth_flag    userlevel;
    char         name[256];
} tcache_node;

typedef struct tokencache_s {
    tcache_node          nodes[256];
    tcache_node          invalids[16];
    int                  count;
    pthread_rwlock_t     lock;
} tokencache;

extern tokencache TOKENCACHE;

void         tokencache_init (void);
tcache_node *tokencache_lookup (const char *token);
void         tokencache_expire (void);
void         tokencache_store_invalid (const char *token);
void         tokencache_store_valid (const char *token, uuid tenantid,
                                     auth_flag userlevel, const char *name);

#endif
