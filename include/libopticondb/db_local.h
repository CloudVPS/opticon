#ifndef _DB_LOCAL_H
#define _DB_LOCAL_H 1

#include <libopticondb/db.h>
#include <libopticon/codec_pkt.h>
#include <libopticon/datatypes.h>

/* =============================== TYPES =============================== */

/** Class structure for localdb */
typedef struct localdb_s {
    db               db; /** Super */
    char            *path; /** Access path for opened handle */
    char            *pathprefix; /** Global prefix for databases */
    codec           *codec; /** Codec reference (for reuse) */
} localdb;

/* ============================= FUNCTIONS ============================= */

datestamp    time2date (time_t in);
db          *localdb_create (const char *prefix);

#endif
