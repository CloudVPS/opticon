#ifndef _DATATYPES_H
#define _DATATYPES_H 1

#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

/* =============================== TYPES =============================== */

/* Flag types and masks */
typedef uint64_t        meterid_t;
typedef uint64_t        metertype_t;
typedef uint32_t        status_t;

#define MTYPE_INT       0x0000000000000000
#define MTYPE_FRAC      0x4000000000000000
#define MTYPE_STR       0x8000000000000000
#define MTYPE_GRID      0xc000000000000000

#define MMASK_TYPE      0xc000000000000000
#define MMASK_COUNT     0x0000000000000007
#define MMASK_NAME      0x3ffffffffffffff8

/* UUIDs are normally passed by value */
typedef struct { uint64_t msb; uint64_t lsb; } uuid;

typedef struct { char str[128]; } fstring;

/* Union for within a meter structure */
typedef union {
    uint64_t         *u64;
    double           *frac;
    fstring          *str;
    void             *any;
} meterdata;

/* Structure representing a specific meter bound to a host */
typedef struct meter_s {
    struct meter_s  *next;
    struct meter_s  *prev;

    meterid_t        id;
    time_t           lastmodified;
    int              count;
    meterdata        d;
} meter;

/* Structure representing a monitored host */
typedef struct host_s {
    struct host_s   *next;
    struct host_s   *prev;
    uuid             uuid;
    status_t         status;
    meter           *first;
    meter           *last;
} host; 

/* Structure representing a keystone tenant */
typedef struct tenant_s {
    struct tenant_s *next;
    struct tenant_s *prev;
    host            *first;
    host            *last;
    uuid             uuid;
    char            *key;
} tenant;

/* List of tenants */
typedef struct {
    tenant          *first;
    tenant          *last;
} tenantlist;

/* ============================== GLOBALS ============================== */

extern tenantlist TENANTS;

/* ============================= FUNCTIONS ============================= */

void        id2label (meterid_t id, char *into);
metertype_t id2type (meterid_t id);
uint64_t    idmask (int sz);

void         tenant_init (void);
tenant      *tenant_find (uuid tenantid);
tenant      *tenant_create (uuid tenantid, const char *key);

host        *host_alloc (void);
host        *host_find (uuid tenantid, uuid hostid);

meter       *host_get_meter (host *h, meterid_t id);
meter       *host_set_meter_uint (host *h, meterid_t id, 
                                  unsigned int count,
                                  uint64_t *data);
meter       *host_set_meter_frac (host *h, meterid_t id,
                                  unsigned int count,
                                  double *data);
meter       *host_set_meter_str  (host *h, meterid_t id,
                                  unsigned int count,
                                  const fstring *str);

meter       *meter_next_sibling (meter *m);                               
void         meter_setcount (meter *m, unsigned int count);
void         meter_set_uint (meter *m, unsigned int pos, uint64_t val);
void         meter_set_frac (meter *m, unsigned int pos, double val);
void         meter_set_str (meter *m, unsigned int pos, const char *str);

uint32_t     meter_get_uint (meter *, unsigned int pos);
double       meter_get_frac (meter *, unsigned int pos);
const char  *meter_get_str (meter *, unsigned int pos);

#endif
