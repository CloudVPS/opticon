#ifndef _TENANT_H
#define _TENANT_H 1

#include <libopticon/aes.h>
#include <libopticon/watchlist.h>
#include <libopticon/summary.h>

/* =============================== TYPES =============================== */

/** Structure representing a keystone tenant */
typedef struct tenant_s {
    struct tenant_s     *next; /**< List link */
    struct tenant_s     *prev; /**< List link */
    host                *first; /**< First linked host */
    host                *last; /**< Last linked host */
    uuid                 uuid; /**< The tenant's uuid */
    aeskey               key; /**< Key used for auth packets */
    watchlist            watch; /**< Tenant-specific watchers */
    summaryinfo          summ; /**< Per tenant meter summaries */
    pthread_rwlock_t     lock; /**< Lock */
} tenant;

/** List of tenants */
typedef struct {
    tenant              *first; /**< First tenant in the list */
    tenant              *last; /**< Last tenant in the list */
    pthread_rwlock_t     lock; /**< Lock */
} tenantlist;

typedef enum tenantlock_e {
    TENANT_LOCK_READ,
    TENANT_LOCK_WRITE
} tenantlock;

/* ============================== GLOBALS ============================== */

extern tenantlist TENANTS;

/* ============================= FUNCTIONS ============================= */

void         tenant_init (void);
void         tenant_delete (tenant *);
tenant      *tenant_find (uuid tenantid, tenantlock);
tenant      *tenant_first (tenantlock);
tenant      *tenant_next (tenant *, tenantlock);
void         tenant_done (tenant *);
tenant      *tenant_create (uuid tenantid, aeskey key);

#endif
