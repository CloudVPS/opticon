#ifndef _DB_LOCAL_H
#define _DB_LOCAL_H 1

#include <db.h>

/* =============================== TYPES =============================== */

typedef uint32_t datestamp;

typedef struct localdb_s {
    db               db;
    char            *path;
} localdb;

/* ============================= FUNCTIONS ============================= */

db          *db_open_local (const char *path);

datestamp	 time2date (time_t in);

#endif
