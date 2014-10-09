#ifndef _TENANT_H
#define _TENANT_H 1

#include <libopticon/aes.h>
#include <libopticon/watchlist.h>
#include <libopticon/summary.h>

/* =============================== TYPES =============================== */

/** Structure representing a keystone tenant */
typedef struct tenant_s {
    struct tenant_s *next; /**< List link */
    struct tenant_s *prev; /**< List link */
    host            *first; /**< First linked host */
    host            *last; /**< Last linked host */
    uuid             uuid; /**< The tenant's uuid */
    aeskey           key; /**< Key used for auth packets */
    watchlist        watch; /**< Tenant-specific watchers */
    summaryinfo      summ; /**< Per tenant meter summaries */
} tenant;

/** List of tenants */
typedef struct {
    tenant          *first; /**< First tenant in the list */
    tenant          *last; /**< Last tenant in the list */
} tenantlist;

/* ============================== GLOBALS ============================== */

extern tenantlist TENANTS;

/* ============================= FUNCTIONS ============================= */

void         tenant_init (void);
tenant      *tenant_find (uuid tenantid);
tenant      *tenant_create (uuid tenantid, aeskey key);

#endif
