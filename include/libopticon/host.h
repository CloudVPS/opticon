#ifndef _HOST_H
#define _HOST_H 1

#include <libopticon/watchlist.h>
#include <libopticon/meter.h>
#include <libopticon/uuid.h>
#include <pthread.h>

/* =============================== TYPES =============================== */

struct tenant_s; /* forward declaration */

/** Structure representing a monitored host */
typedef struct host_s {
    struct host_s       *next; /**< List link */
    struct host_s       *prev; /**< List link */
    struct tenant_s     *tenant; /**< Parent link */
    time_t               lastmodified; /**< timeout information */
    uint32_t             lastserial; /**< last received serial */
    uuid                 uuid; /**< uuid of the host */
    double               badness; /**< accumulated badness */
    status_t             status; /**< current status (if relevant */
    adjustlist           adjust; /**< meterwatch adjustments */
    time_t               lastmetasync; /**< Time of last metadata sync-in*/
    meter               *first; /**< first connected meter */
    meter               *last; /**< last connected meter */
    
                         /** Number of active meters. The pktcodec
                           * uses this to keep a reasonable
                           * quota on the number of active meters
                           * a single host is allowed to keep.
                           */
    int                  mcount;
    
    pthread_rwlock_t     lock; /**< Threading infrastructure */
} host;

/* ============================= FUNCTIONS ============================= */

host        *host_alloc (void);
host        *host_find (uuid tenantid, uuid hostid);
void         host_delete (host *);

void         host_delete_meter (host *h, meter *m);
void         host_begin_update (host *h, time_t when);
void         host_end_update (host *h);
meter       *host_find_meter_name (host *h, meterid_t id);
meter       *host_find_meter (host *h, meterid_t id);
meter       *host_get_meter (host *h, meterid_t id);
int          host_has_meter (host *h, meterid_t id);
meter       *host_set_meter_uint (host *h, meterid_t id, 
                                  unsigned int count,
                                  uint64_t *data);
meter       *host_set_meter_frac (host *h, meterid_t id,
                                  unsigned int count,
                                  double *data);
meter       *host_set_meter_str  (host *h, meterid_t id,
                                  unsigned int count,
                                  const fstring *str);
meter       *host_find_prefix (host *h, meterid_t prefix, meter *prev);

#endif
