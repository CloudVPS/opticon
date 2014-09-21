#include "opticon-collector.h"
#include <libopticondb/db_local.h>
#include <libopticon/auth.h>
#include <libopticon/ioport.h>
#include <libopticon/pktwrap.h>
#include <libopticon/util.h>
#include <libopticonf/var.h>
#include <libopticonf/parse.h>
#include <libopticonf/react.h>
#include <libsvc/daemon.h>
#include <libsvc/log.h>
#include <libsvc/cliopt.h>
#include <libsvc/transport_udp.h>

/** Structure for inline definition of a meterwatch */
typedef struct watchdef_s {
    const char  *id;
    watchtype    tp;
    const char  *watchstr;
    double       badness;
} watchdef;

watchdef DEFAULT_WATCHERS[] = {
    {"pcpu",        WATCH_FRAC_GT,  "80.0",   3.0},
    {"pcpu",        WATCH_FRAC_GT,  "98.0",  10.0},
    {"loadavg",     WATCH_FRAC_GT,  "8.0",    3.0},
    {"loadavg",     WATCH_FRAC_GT,  "25.0",  12.0},
    {"mem/free",    WATCH_UINT_LT,  "65536",  5.0},
    {"swap/free",   WATCH_UINT_LT,  "65536", 15.0},
    {"piowait",     WATCH_FRAC_GT,  "50.0",   5.0},
    {"piowait",     WATCH_FRAC_GT,  "80.0",  10.0},
    {NULL,          0,              NULL,     0.0}
};

watchlist WATCHERS;

/** Use a meterwatch definition to turn a meter value into a badness
  * number.
  * \param m The meter to inspect
  * \param w The meterwatch definition to use.
  * \return A badness number, accumulated for all array nodes.
  */
double calculate_badness (meter *m, meterwatch *w) {
    double res = 0.0;
    fstring tstr;
    
    for (int i=0; i<((m->count)?m->count:1); ++i) {
        switch (w->tp) {
            case WATCH_FRAC_GT:
                if (meter_get_frac (m, i) > w->dat.frac) res += w->badness;
                break;
            
            case WATCH_FRAC_LT:
                if (meter_get_frac (m, i) < w->dat.frac) res += w->badness;
                break;

            case WATCH_UINT_GT:
                if (meter_get_uint (m, i) > w->dat.u64) res += w->badness;
                break;
            
            case WATCH_UINT_LT:
                if (meter_get_uint (m, i) < w->dat.u64) res += w->badness;
                break;
            
            case WATCH_STR_MATCH:
                tstr = meter_get_str (m, i);
                if (strcmp (tstr.str, w->dat.str.str) == 0) {
                    res += w->badness;
                }
                break;
            
            default:
                break;
        }
    }
    return res;
}

/** Inspect a host's metering data to determine its current status
  * and problems, then write it to disk.
  * \param host The host to inspect.
  */
void watchthread_handle_host (host *host) {
    int problemcount = 0;
    time_t tnow = time (NULL);
    meter *m = host->first;
    meterwatch *w;
    char label[16];
    
    pthread_rwlock_wrlock (&host->lock);

    /* We'll store the status information as a meter itself */
    meterid_t mid_status = makeid ("status",MTYPE_STR,0);
    meter *m_status = host_get_meter (host, mid_status);
    meter_setcount (m_status, 0);
    
    /* If the data is stale, don't add to badness, just set the status. */
    if ((tnow - host->lastmodified) > 80) {
        meter_set_str (m_status, 0, "STALE");
        pthread_rwlock_unlock (&host->lock);
        return;
    }

    /* Figure out badness for each meter, and add to host */
    while (m) {
        m->badness = 0.0;
        int handled = 0;
        
        w = host->tenant->watch.first;
        while (w) {
            if (w->id == (m->id & MMASK_NAME)) {
                m->badness += calculate_badness (m, w);
                handled = 1;
            }
            w = w->next;
        }
        
        if (! handled) w = APP.watch.first;
         while (w) {
            if (w->id == (m->id & MMASK_NAME)) {
                m->badness += calculate_badness (m, w);
            }
            w = w->next;
        }
       
        if (m->badness) problemcount++;
        host->badness += m->badness;
    }
    
    /* Put up the problems as a meter as well */
    meterid_t mid_problems = makeid ("problems",MTYPE_STR,0);
    meter *m_problems = host_get_meter (host, mid_problems);
    
    if (! problemcount) {
        if (host->badness > 100.0) host->badness = host->badness/2.0;
        else if (host->badness > 10.0) host->badness -= 10.0;
        else if (host->badness > 1.0) host->badness = host->badness *0.7;
        else host->badness = 0.0;
        meter_setcount (m, 0);
        meter_set_str (m, 0, "none");
    }
    else {
        int i=0;
        meter_setcount (m_problems, problemcount);
        m = host->first;
        
        while (m && (i<16)) {
            if (m->badness > 0.00) {
                id2str (m->id, label);
                meter_set_str (m_problems, i++, label);
            }
            m = m->next;
        }
    }
    
    /* Convert badness to a status text */
    if (host->badness < 30.0) {
        meter_set_str (m_status, 0, "OK");
    }
    else if (host->badness < 80.0) {
        meter_set_str (m_status, 0, "WARN");
    }
    else (meter_set_str (m_status, 0, "ALERT"));
    
    /* Write to db */
    if (db_open (APP.writedb, host->tenant->uuid, NULL)) {
        db_save_record (APP.writedb, tnow, host);
        db_close (APP.writedb);
    }
    
    pthread_rwlock_unlock (&host->lock);
}

/** Main loop for the watchthread */
void watchthread_run (thread *self) {
    time_t t_start, t_end;
    tenant *tcrsr;
    host *hcrsr;
    
    while (1) {
        t_start = time (NULL);
        tcrsr = TENANTS.first;
        while (tcrsr) {
            hcrsr = tcrsr->first;
            while (hcrsr) {
                watchthread_handle_host (hcrsr);
                hcrsr = hcrsr->next;
            }
            tcrsr = tcrsr->next;
        }
        t_end = time (NULL);
        if (t_end - t_start < 60) {
            log_info ("Watchthread took %i seconds", t_end-t_start);
            sleep (60 - (t_end-t_start));
        }
        else {
            log_error ("Watchthread round cannot keep up");
        }
    }
}
