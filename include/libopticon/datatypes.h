#ifndef _DATATYPES_H
#define _DATATYPES_H 1

#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libopticon/uuid.h>
#include <libopticon/aes.h>
#include <libopticon/meter.h>
#include <libopticon/watchlist.h>
#include <libopticon/host.h>
#include <libopticon/tenant.h>

/* =============================== TYPES =============================== */


/* ============================= FUNCTIONS ============================= */

void        id2label (meterid_t id, char *into);
metertype_t id2type (meterid_t id);
uint64_t    idmask (int sz);

#endif
