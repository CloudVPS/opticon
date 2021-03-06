#ifndef _OPTICON_COLLECTOR_H
#define _OPTICON_COLLECTOR_H 1

#include <libopticon/datatypes.h>
#include <libopticon/codec.h>
#include <libopticondb/db.h>
#include <libopticon/var.h>
#include <libopticon/packetqueue.h>
#include <libopticon/summary.h>
#include <libopticon/transport.h>
#include <libopticon/thread.h>

/* =============================== TYPES =============================== */

typedef struct conf_reloader_s {
    thread       super;
    conditional *cond;
} conf_reloader;

/** Useful access to application parts and configuration */
typedef struct appcontext_s {
    codec           *codec; /* The packet codec */
    db              *db; /* Generic db handle */
    db              *writedb; /* Handle for host writes to db */
    db              *overviewdb; /* Handle for overview thread */
    db              *reaperdb; /* Handle for reaper thread */
    packetqueue     *queue; 
    intransport     *transport;
    watchlist        watch;
    thread          *watchthread;
    thread          *overviewthread;
    thread          *reaperthread;
    conf_reloader   *reloader;
    var             *conf;
    int              loglevel;
    const char      *logpath;
    const char      *confpath;
    const char      *mconfpath;
    const char      *pidfile;
    int              foreground;
    int              listenport;
    const char      *listenaddr;
} appcontext;

/* ============================== GLOBALS ============================== */

extern appcontext APP;

/* ============================= FUNCTIONS ============================= */

void watchthread_run (thread *self);
void overviewthread_run (thread *self);
void conf_reloader_run (thread *);
void reaper_run (thread *self);
conf_reloader *conf_reloader_create (void);
void conf_reloader_reload (conf_reloader *);

#endif
