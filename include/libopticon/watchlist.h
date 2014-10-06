#ifndef _WATCHLIST_H
#define _WATCHLIST_H 1

#include <libopticon/meter.h>
#include <pthread.h>

/* =============================== TYPES =============================== */

/** Distinguish different type of meterwatch matching */
typedef enum watchtype_e {
    WATCH_FRAC_LT, /** Match on a fractional value being less than N */
    WATCH_FRAC_GT, /** match on a fractional value being greater than N */
    WATCH_UINT_LT, /** Match on an integer value being less than N */
    WATCH_UINT_GT, /** Match on an integer value being greater than N */
    WATCH_STR_MATCH /** Match on a specific string value */
} watchtype;

/** Data type indicator for a host-bound watch adjustment */
typedef enum watchadjusttype_e {
    WATCHADJUST_NONE,
    WATCHADJUST_FRAC,
    WATCHADJUST_UINT,
    WATCHADJUST_STR
} watchadjusttype;

/** Trigger level for a watch or adjustment */
typedef enum watchtrigger_e {
    WATCH_NONE,
    WATCH_WARN,
    WATCH_ALERT,
    WATCH_CRIT
} watchtrigger;

/** Storage for the match data of a meterwatch object */
typedef union {
    uint64_t         u64;
    double           frac;
    fstring          str;
} watchdata;

/** Represents an adjustment to a meterwatch at a specific alert level */
typedef struct adjustdata_s {
    watchdata        data;
    double           weight;
} adjustdata;

/** Represents a meterwatch adjustment at the host level */
typedef struct watchadjust_s {
    struct watchadjust_s    *next;
    struct watchadjust_s    *prev;
    watchadjusttype          type;
    meterid_t                id;
    adjustdata               adjust[WATCH_CRIT];
} watchadjust;

/** List header for a list of watchadjust objects */
typedef struct adjustlist_s {
    watchadjust         *first;
    watchadjust         *last;
} adjustlist;

/** Definition of a match condition that indicates a problem
  * for a host.
  */
typedef struct meterwatch_s {
    struct meterwatch_s *next; /**< List neighbour */
    struct meterwatch_s *prev; /**< List neighbour */
    watchtrigger         trigger; /**< Alert level trigger type */
    meterid_t            id; /** id to match */
    watchtype            tp; /** Evaluation function to use */
    watchdata            dat; /** Match argument */
    double               badness; /** Impact weight */
} meterwatch;

/** Linked list header of a meterwatch list */
typedef struct watchlist_s {
    meterwatch          *first;
    meterwatch          *last;
    pthread_mutex_t      mutex;
} watchlist;

/* ============================= FUNCTIONS ============================= */

void         watchlist_init (watchlist *);
void         watchlist_add_uint (watchlist *, meterid_t, watchtype,
                                 uint64_t, double, watchtrigger);
void         watchlist_add_frac (watchlist *, meterid_t, watchtype,
                                 double, double, watchtrigger);
void         watchlist_add_str (watchlist *, meterid_t, watchtype,
                                const char *, double, watchtrigger);
void         watchlist_clear (watchlist *);

void         adjustlist_init (adjustlist *);
void         adjustlist_clear (adjustlist *);
watchadjust *adjustlist_find (adjustlist *, meterid_t id);
watchadjust *adjustlist_get (adjustlist *, meterid_t id);

#endif
