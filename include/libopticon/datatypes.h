#ifndef _DATATYPES_H
#define _DATATYPES_H 1

#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

/* =============================== TYPES =============================== */

/** Storage for an AES256 key */
typedef struct aeskey_s {
    uint8_t data[32];
} aeskey;

/* Flag types and masks */
typedef uint64_t        meterid_t;
typedef uint64_t        metertype_t;
typedef uint32_t        status_t;

#define MTYPE_INT       0x0000000000000000
#define MTYPE_FRAC      0x4000000000000000
#define MTYPE_STR       0x8000000000000000
#define MTYPE_GRID      0xc000000000000000

#define MMASK_TYPE      0xc000000000000000
#define MMASK_COUNT     0x000000000000000f
#define MMASK_NAME      0x3ffffffffffffff0

/** UUIDs are normally passed by value */
typedef struct { uint64_t msb; uint64_t lsb; } uuid;

typedef struct { char str[128]; } fstring;

/** Union for within a meter structure */
typedef union {
    uint64_t         *u64; /**< Integer representation */
    double           *frac; /**< Fractional representation */
    fstring          *str; /**< String representation */
    void             *any; /**< Raw pointer */
} meterdata;

struct host_s; /* forward declaration */

/** Structure representing a specific meter bound to a host */
typedef struct meter_s {
    struct meter_s  *next; /**< List link */
    struct meter_s  *prev; /**< List link */
    struct host_s   *host; /**< Parent link */

    meterid_t        id; /**< id and type of the data */
    time_t           lastmodified; /**< timeout information */
    int              count; /**< Element count, 0 means direct value */
    meterdata        d; /**< value */
} meter;

struct tenant_s; /* forward declaration */

/** Structure representing a monitored host */
typedef struct host_s {
    struct host_s   *next; /**< List link */
    struct host_s   *prev; /**< List link */
    struct tenant_s *tenant; /**< Parent link */
    time_t           lastmodified; /**< timeout information */
    uuid             uuid; /**< uuid of the host */
    status_t         status; /**< current status (if relevant */
    meter           *first; /**< first connected meter */
    meter           *last; /**< last connected meter */
} host; 

/** Structure representing a keystone tenant */
typedef struct tenant_s {
    struct tenant_s *next; /**< List link */
    struct tenant_s *prev; /**< List link */
    host            *first; /**< First linked host */
    host            *last; /**< Last linked host */
    uuid             uuid; /**< The tenant's uuid */
    aeskey           key; /**< Key used for auth packets */
} tenant;

/** List of tenants */
typedef struct {
    tenant          *first; /**< First tenant in the list */
    tenant          *last; /**< Last tenant in the list */
} tenantlist;

/* ============================== GLOBALS ============================== */

extern tenantlist TENANTS;

/* ============================= FUNCTIONS ============================= */

void        id2label (meterid_t id, char *into);
metertype_t id2type (meterid_t id);
uint64_t    idmask (int sz);

void         tenant_init (void);
tenant      *tenant_find (uuid tenantid);
tenant      *tenant_create (uuid tenantid, aeskey key);

host        *host_alloc (void);
host        *host_find (uuid tenantid, uuid hostid);
void         host_delete (host *);

void         host_begin_update (host *h, time_t when);
void         host_end_update (host *h);
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

meter       *meter_next_sibling (meter *m);                               
void         meter_setcount (meter *m, unsigned int count);
void         meter_set_uint (meter *m, unsigned int pos, uint64_t val);
void         meter_set_frac (meter *m, unsigned int pos, double val);
void         meter_set_str (meter *m, unsigned int pos, const char *str);

uint64_t     meter_get_uint (meter *, unsigned int pos);
double       meter_get_frac (meter *, unsigned int pos);
const char  *meter_get_str (meter *, unsigned int pos);

#endif
