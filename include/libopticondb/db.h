#ifndef _DB_H
#define _DB_H 1

#include <time.h>
#include <libopticon/datatypes.h>
#include <libopticonf/var.h>

/* =============================== TYPES =============================== */

struct db_s; /* forward declaration */

typedef int (*open_db_f)(struct db_s *, uuid, var *);
typedef int (*get_record_f)(struct db_s *, time_t, host *);
typedef uint64_t *(*get_vrangei_f)(struct db_s *, time_t, time_t, int,
                                   meterid_t, uint8_t, host *);
typedef double *(*get_vrangef_f)(struct db_s *, time_t, time_t, int,
                                 meterid_t, uint8_t, host *);
typedef int (*save_record_f)(struct db_s *, time_t, host *);
typedef uuid *(*list_hosts_f)(struct db_s *, int *);
typedef var *(*get_metadata_f)(struct db_s *);
typedef int (*set_metadata_f)(struct db_s *, var *);
typedef void (*close_db_f)(struct db_s *);
typedef int (*create_tenant_f)(struct db_s *, uuid, var *);
typedef int (*remove_tenant_f)(struct db_s *, uuid);
typedef void (*free_db_f)(struct db_s *);

/** Database handle virtual class */
typedef struct db_s {
    open_db_f        open; /** Unbound method */
    get_record_f     get_record; /** Method */
    get_vrangei_f    get_value_range_int; /** Method */
    get_vrangef_f    get_value_range_frac; /** Method */
    save_record_f    save_record; /** Method */
    list_hosts_f     list_hosts; /** Method */
    get_metadata_f   get_metadata; /** Method */
    set_metadata_f   set_metadata; /** Method */
    close_db_f       close; /** Method */
    create_tenant_f  create_tenant; /** Unbound method */
    remove_tenant_f  remove_tenant; /** Unbound method */
    free_db_f        free; /** Unbound method */
    uuid             tenant; /** The bound tenant */
    int              opened; /** 1 if currently bound to a host */
} db;

/* ============================= FUNCTIONS ============================= */

int          db_open (db *d, uuid tenant, var *extra);
int          db_get_record (db *d, time_t when, host *into);
uint64_t    *db_get_value_range_int (db *d, time_t start, time_t end,
                                     int numsamples, meterid_t key,
                                     uint8_t arrayindex, host *host);
double      *db_get_value_range_frac (db *d, time_t start, time_t end,
                                      int numsamples, meterid_t key,
                                      uint8_t arrayindex, host *host);
int          db_save_record (db *d, time_t when, host *what);
uuid        *db_list_hosts (db *d, int *outsz);
var         *db_get_metadata (db *d);
int          db_set_metadata (db *d, var *v);
void         db_close (db *d);
int          db_create_tenant (db *d, uuid tuuid, var *meta);
int          db_remove_tenant (db *d, uuid u);

#endif
