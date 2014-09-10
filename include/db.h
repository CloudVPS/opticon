#ifndef _DB_H
#define _DB_H 1

#include <time.h>
#include <datatypes.h>

/* =============================== TYPES =============================== */

struct db_s;

typedef int (*get_record_f)(struct db_s *, time_t, host *);
typedef uint64_t *(*get_vrangei_f)(struct db_s *, time_t, time_t, int,
                                   meterid_t, uint8_t, host *);
typedef double *(*get_vrangef_f)(struct db_s *, time_t, time_t, int,
                                 meterid_t, uint8_t, host *);
typedef int (*save_record_f)(struct db_s *, time_t, host *);
typedef void (*close_db_f)(struct db_s *);

typedef struct db_s {
    get_record_f     get_record;
    get_vrangei_f    get_value_range_int;
    get_vrangef_f    get_value_range_frac;
    save_record_f    save_record;
    close_db_f       close;
} db;

/* ============================= FUNCTIONS ============================= */

int          db_get_record (db *d, time_t when, host *into);
uint64_t    *db_get_value_range_int (db *d, time_t start, time_t end,
                                     int numsamples, meterid_t key,
                                     uint8_t arrayindex, host *host);
double      *db_get_value_range_frac (db *d, time_t start, time_t end,
                                      int numsamples, meterid_t key,
                                      uint8_t arrayindex, host *host);
int          db_save_record (db *d, time_t when, host *what);
void         db_close (db *d);

#endif
