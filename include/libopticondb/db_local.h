#ifndef _DB_LOCAL_H
#define _DB_LOCAL_H 1

#include <libopticondb/db.h>
#include <libopticon/codec.h>
#include <libopticon/datatypes.h>

/* =============================== TYPES =============================== */

/** Representation of a date as a YYYYMMDD integer */
typedef uint32_t datestamp;

/** Class structure for localdb */
typedef struct localdb_s {
    db               db; /** Super */
    char            *path; /** Base path for this handle */
    codec           *codec; /** Codec reference (for reuse) */
} localdb;

/* ============================= FUNCTIONS ============================= */

db          *db_open_local (const char *path, uuid u);

datestamp    time2date (time_t in);

#endif
