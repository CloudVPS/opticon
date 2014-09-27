#include "opticon-collector.h"
#include <libopticondb/db_local.h>
#include <libopticon/auth.h>
#include <libopticon/ioport.h>
#include <libopticon/pktwrap.h>
#include <libopticon/util.h>
#include <libopticon/var.h>
#include <libopticon/parse.h>
#include <libopticon/react.h>
#include <libopticon/daemon.h>
#include <libopticon/log.h>
#include <libopticon/cliopt.h>
#include <libopticon/transport_udp.h>

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
double calculate_badness (meter *m, meterwatch *w, watchtrigger *maxtrig) {
    double res = 0.0;
    fstring tstr;
    
    for (int i=0; i<((m->count)?m->count:1); ++i) {
        switch (w->tp) {
            case WATCH_FRAC_GT:
                if (meter_get_frac (m, i) > w->dat.frac) {
                    res += w->badness;
                    if (w->trigger > *maxtrig) *maxtrig = w->trigger;
                }
                break;
            
            case WATCH_FRAC_LT:
                if (meter_get_frac (m, i) < w->dat.frac) {
                    res += w->badness;
                    if (w->trigger > *maxtrig) *maxtrig = w->trigger;
                }
                break;

            case WATCH_UINT_GT:
                if (meter_get_uint (m, i) > w->dat.u64) {
                    res += w->badness;
                    if (w->trigger > *maxtrig) *maxtrig = w->trigger;
                }
                break;
            
            case WATCH_UINT_LT:
                if (meter_get_uint (m, i) < w->dat.u64) {
                    res += w->badness;
                    if (w->trigger > *maxtrig) *maxtrig = w->trigger;
                }
                break;
            
            case WATCH_STR_MATCH:
                tstr = meter_get_str (m, i);
                if (strcmp (tstr.str, w->dat.str.str) == 0) {
                    res += w->badness;
                    if (w->trigger > *maxtrig) *maxtrig = w->trigger;
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
    double totalbadness = 0.0;
    time_t tnow = time (NULL);
    meter *m = host->first;
    meterwatch *w;
    watchtrigger maxtrigger = WATCH_NONE;
    char label[16];
    char uuidstr[40];
    
    pthread_rwlock_wrlock (&host->lock);

    /* We'll store the status information as a meter itself */
    meterid_t mid_status = makeid ("status",MTYPE_STR,0);
    meter *m_status = host_get_meter (host, mid_status);
    fstring ostatus = meter_get_str (m_status, 0);
    meter_setcount (m_status, 0);
    if (ostatus.str[0] == 0) strcpy (ostatus.str, "UNSET");
    
    /* If the data is stale, don't add to badness, just set the status. */
    if ((tnow - host->lastmodified) > 80) {
        if (strcmp (ostatus.str, "STALE") != 0) {
            uuid2str (host->uuid, uuidstr);
            log_info ("Status change host %s %s -> STALE", uuidstr, ostatus.str);
        }
        meter_set_str (m_status, 0, "STALE");
    }
    else {

        /* Figure out badness for each meter, and add to host */
        while (m) {
            m->badness = 0.0;
            int handled = 0;
        
            /* First go over the tenant-defined watchers */
            pthread_mutex_lock (&host->tenant->watch.mutex);
            w = host->tenant->watch.first;
            while (w) {
                if ((w->id & MMASK_NAME) == (m->id & MMASK_NAME)) {
                    m->badness += calculate_badness (m, w, &maxtrigger);
                    handled = 1;
                }
                w = w->next;
            }
            pthread_mutex_unlock (&host->tenant->watch.mutex);
       
            /* If the tenant didn't have anything suitable, go over the
               global watchlist */
            if (! handled) {
                pthread_mutex_lock (&APP.watch.mutex);
                w = APP.watch.first;
                /* First go over the tenant-defined watchers */
                w = host->tenant->watch.first;
                while (w) {
                    if ((w->id & MMASK_NAME) == (m->id & MMASK_NAME)) {
                        m->badness += calculate_badness (m, w, &maxtrigger);
                        handled = 1;
                    }
                    w = w->next;
                }
       
                /* If the tenant didn't have anything suitable, go over the
                   global watchlist */
                if (! handled) w = APP.watch.first;
                while (w) {
                    if ((w->id & MMASK_NAME) == (m->id & MMASK_NAME)) {
                        m->badness += calculate_badness (m, w, &maxtrigger);
                    }
                    w = w->next;
                }
                pthread_mutex_unlock (&APP.watch.mutex);
            }
      
            if (m->badness) problemcount++;
            totalbadness += m->badness;
            m = m->next;
        }
    
        /* Don't raise a CRIT alert on a WARN condition */
        switch (maxtrigger) {
            case WATCH_NONE:
                totalbadness = 0.0;
                break;
        
            case WATCH_WARN:
                if (host->badness > 50.0) totalbadness = 0.0;
                break;
        
            case WATCH_ALERT:
                if (host->badness > 90.0) totalbadness = 0.0;
                break;
        
            case WATCH_CRIT:
                if (host->badness > 150.0) totalbadness = 0.0;
                break;
        }
    
        host->badness += totalbadness;
    
        /* Put up the problems as a meter as well */
        meterid_t mid_problems = makeid ("problems",MTYPE_STR,0);
        meter *m_problems = host_get_meter (host, mid_problems);
    
        /* While we're looking at it, consider the current badness, if there
           are no problems, it should be going down. */
        if (! problemcount) {
            if (host->badness > 100.0) host->badness = host->badness/2.0;
            else if (host->badness > 1.0) host->badness = host->badness *0.75;
            else host->badness = 0.0;
            meter_set_empty_array (m_problems);
        }
        else {
            /* If we reached the top, current level may still be out of
               its league, so allow it to decay slowly */
            if (totalbadness == 0.0) host->badness *= 0.9;
        
            /* Fill in the problem array */
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
    
        meterid_t mid_badness = makeid ("badness",MTYPE_FRAC,0);
        meter *m_badness = host_get_meter (host, mid_badness);
        meter_setcount (m_badness, 0);
        meter_set_frac (m_badness, 0, host->badness);
    
        const char *nstatus = "UNSET";
    
        /* Convert badness to a status text */
        if (host->badness < 30.0) nstatus = "OK";
        else if (host->badness < 80.0) nstatus = "WARN";
        else if (host->badness < 120.0) nstatus = "ALERT";
        else nstatus = "CRIT";
    
        if (strcmp (nstatus, ostatus.str) != 0) {
            uuid2str (host->uuid, uuidstr);
            log_info ("Status change host %s %s -> %s", uuidstr, ostatus.str, nstatus);
        }
    
        meter_set_str (m_status, 0, nstatus);
    }
    
    /* Write to db */
    if (db_open (APP.writedb, host->tenant->uuid, NULL)) {
        db_save_record (APP.writedb, tnow, host);
        db_close (APP.writedb);
    }
    
    pthread_rwlock_unlock (&host->lock);
}

/** Main loop for the watchthread */
void watchthread_run (thread *self) {
    tenant *tcrsr;
    host *hcrsr;
    time_t t_now = time (NULL);
    time_t t_next = (t_now+60)-((t_now+60)%60);
    log_debug ("Watchthread started");
    sleep (t_next - t_now);
    t_next += 60;
    
    while (1) {
        tcrsr = TENANTS.first;
        while (tcrsr) {
            hcrsr = tcrsr->first;
            while (hcrsr) {
                watchthread_handle_host (hcrsr);
                hcrsr = hcrsr->next;
            }
            tcrsr = tcrsr->next;
        }
        
        t_now = time (NULL);
        if (t_now < t_next) {
            log_debug ("Watchthread took %i seconds", 60-(t_next-t_now));
            sleep (t_next-t_now);
        }
        else {
            log_error ("Watchthread round cannot keep up");
        }
        t_next += 60;
        while (t_next < t_now) t_next += 60;
    }
}
