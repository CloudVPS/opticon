#ifndef _METER_H
#define _METER_H 1

#include <stdint.h>
#include <time.h>

/* =============================== TYPES =============================== */

/* Flag types and masks */
typedef uint64_t        meterid_t;
typedef uint64_t        metertype_t;
typedef uint32_t        status_t;

#define MTYPE_INT       0x0000000000000000
#define MTYPE_FRAC      0x4000000000000000
#define MTYPE_STR       0x8000000000000000

#define MMASK_TYPE      0xc000000000000000
#define MMASK_COUNT     0x000000000000001f
#define MMASK_NAME      0x3fffffffffffffe0

#define SZ_MAX_ARRAY    0x000000000000001d
#define SZ_EMPTY_VAL    0x000000000000001e
#define SZ_EMPTY_ARRAY  0x000000000000001f

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

    double           badness; /**< accumulated badness */
    meterid_t        id; /**< id and type of the data */
    time_t           lastmodified; /**< timeout information */
    int              count; /**< Element count, 0 means direct value */
    meterdata        d; /**< value */
} meter;

/* ============================= FUNCTIONS ============================= */

meter       *meter_alloc (void);
meter       *meter_next_sibling (meter *m);
void         meter_set_empty (meter *m);
void         meter_set_empty_array (meter *m);
void         meter_setcount (meter *m, unsigned int count);
void         meter_set_uint (meter *m, unsigned int pos, uint64_t val);
void         meter_set_frac (meter *m, unsigned int pos, double val);
void         meter_set_str (meter *m, unsigned int pos, const char *str);

uint64_t     meter_get_uint (meter *, unsigned int pos);
double       meter_get_frac (meter *, unsigned int pos);
fstring      meter_get_str (meter *, unsigned int pos);

#endif
