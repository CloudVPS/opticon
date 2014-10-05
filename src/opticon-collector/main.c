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
#include <libopticon/defaultmeters.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>

/** Look up a session by netid and sessionid. If it's a valid session,
  * return its current AES session key.
  * \param netid The host's netid (32 bit ip-derived).
  * \param sid The session-id.
  * \param serial Serial-number of packet being handled.
  * \param blob Pointer for giving back the session.
  * \return The key, or NULL if resolving failed, or session was invalid.
  */
aeskey *resolve_sessionkey (uint32_t netid, uint32_t sid, uint32_t serial,
                            void **blob) {
    session *S = session_find (netid, sid);
    if (! S) {
        log_warn ("Session <%08x-%08x> not found", sid, netid);
        return NULL;
    }
    if (S->lastserial >= serial) {
        log_warn ("Rejecting old serial <%i> for session <%08x-%08x>",
                  serial, sid, netid);
        return NULL;
    }
    if (S->host->lastserial >= serial) {
        log_warn ("Rejecting old serial <%i> for session <%08x-%08x>",
                  serial, sid, netid);
        return NULL;
    }
    S->host->lastserial = serial;
    *blob = (void *)S;
    return &S->key;
}

/** Add a meterwatch to a (tenant's) watchlist, with a var object
  * as configuration; helper for watchlist_populate().
  * \param w The watchlist to add to.
  * \param id The meterid to match
  * \param mtp The metertype to match
  * \param v Metadata, dict with: cmp, weight, val.
  * \param vweight The unweighted 'badness' (depending on alert/warning).
  */
void make_watcher (watchlist *w, meterid_t id, metertype_t mtp,
                   var *v, double vweight, watchtrigger tr) {
    const char *cmp = var_get_str_forkey (v, "cmp");
    double weight = var_get_double_forkey (v, "weight");
    uint64_t ival = var_get_int_forkey (v, "val");
    double dval = var_get_double_forkey (v, "val");
    const char *sval = var_get_str_forkey (v, "val");
    
    if (weight < 0.01) weight = 1.0;
    weight = weight * vweight;
    
    if (mtp == MTYPE_INT) {
        if (strcmp (cmp, "lt") == 0) {
            watchlist_add_uint (w, id, WATCH_UINT_LT, ival, weight, tr);
        }
        else if (strcmp (cmp, "gt") == 0) {
            watchlist_add_uint (w, id, WATCH_UINT_GT, ival, weight, tr);
        }
    }
    else if (mtp == MTYPE_FRAC) {
        if (strcmp (cmp, "lt") == 0) {
            watchlist_add_frac (w, id, WATCH_FRAC_LT, dval, weight, tr);
        }
        else if (strcmp (cmp, "gt") == 0) {
            watchlist_add_frac (w, id, WATCH_FRAC_GT, dval, weight, tr);
        }
    }
    else if (mtp == MTYPE_STR) {
        if (strcmp (cmp, "eq") == 0) {
            watchlist_add_str (w, id, WATCH_STR_MATCH, sval, weight, tr);
        }
    }
}

/** Populate a watchlist out of a meter definition stored in some
  * variables.
  * \param w The watchlist to populate.
  * \param v_meters The meter definitions.
  */
void watchlist_populate (watchlist *w, var *v_meters) {
    pthread_mutex_lock (&w->mutex);
    watchlist_clear (w);
    if (v_meters) {
        var *mdef = v_meters->value.arr.first;
        while (mdef) {
            var *v_warn = var_get_dict_forkey (mdef, "warning");
            var *v_alert = var_get_dict_forkey (mdef, "alert");
            var *v_crit = var_get_dict_forkey (mdef, "critical");
            const char *type = var_get_str_forkey (mdef, "type");
            if (type && (v_warn || v_alert || v_crit)) {
                metertype_t tp;
                if (memcmp (type, "int", 3) == 0) tp = MTYPE_INT;
                else if (strcmp (type, "frac") == 0) tp = MTYPE_FRAC;
                else if (memcmp (type, "str", 3) == 0) tp = MTYPE_STR;
                else {
                    /* Skip table, complain about others */
                    if (strcmp (type, "table") != 0) {
                        log_error ("Meter configured with unrecognized "
                                   "typeid %s", type);
                    }
                    mdef = mdef->next;
                    continue;
                }
                
                meterid_t id = makeid (mdef->id, tp, 0);
                if (var_get_count (v_warn)) {
                     make_watcher (w, id, tp, v_warn, 5.0, WATCH_WARN);
                }
                if (var_get_count (v_alert)) {
                     make_watcher (w, id, tp, v_alert, 5.0, WATCH_ALERT);
                }
                if (var_get_count (v_crit)) {
                    make_watcher (w, id, tp, v_crit, 10.0, WATCH_CRIT);
                }
            }
            mdef = mdef->next;
        }
    }
    pthread_mutex_unlock (&w->mutex);
}

/** Look up a tenant in memory and in the database, do the necessary
  * bookkeeping, then returns the AES key for that tenant.
  * \param tenantid The tenant UUID.
  * \param serial Serial number of the auth packet we're dealing with.
  * \return Pointer to the key, or NULL if resolving failed.
  */
aeskey *resolve_tenantkey (uuid tenantid, uint32_t serial) {
    tenant *T = tenant_find (tenantid);
    var *meta;
    const char *b64;
    aeskey *res = NULL;
    
    /* Discard tenants we can't find in the database */
    if (! db_open (APP.db, tenantid, NULL)) {
        // FIXME remove tenant
        log_error ("(resolve_tenantkey) Packet for unknown tenantid");
        return NULL;
    }
    
    /* Discard tenants with no metadata */
    if (! (meta = db_get_metadata (APP.db))) {
        log_error ("(resolve_tenantkey) Error reading metadata");
        db_close (APP.db);
        return NULL;
    }
    
    /* Find the 'key' metavalue */
    if ((b64 = var_get_str_forkey (meta, "key"))) {
        if (T) {
            /* Update the existing tenant structure with information
               from the database */
            T->key = aeskey_from_base64 (b64);
            res = &T->key;
        }
        else {
            /* Create a new in-memory tenant */
            T = tenant_create (tenantid, aeskey_from_base64 (b64));
            if (T) {
                res = &T->key;
            }
        }
    }

    /* If there's meter data defined in the tenant metadata,
       put it in the tenant's watchlist */    
    var *v_meters = var_get_dict_forkey (meta, "meter");
    if (v_meters) watchlist_populate (&T->watch, v_meters);
    
    db_close (APP.db);
    var_free (meta);
    return res;
}

/** Handler for an auth packet */
void handle_auth_packet (ioport *pktbuf, uint32_t netid,
                         struct sockaddr_storage *remote) {
    authinfo *auth = ioport_unwrap_authdata (pktbuf, resolve_tenantkey);
    
    char addrbuf[INET6_ADDRSTRLEN];
    ip2str (remote, addrbuf);
    
    /* No auth means discarded by the crypto/decoding layer */
    if (! auth) {
        log_warn ("Authentication failed on packet from <%s>", addrbuf);
        return;
    }
    
    char s_hostid[40];
    char s_tenantid[40];
    
    uuid2str (auth->hostid, s_hostid);
    uuid2str (auth->tenantid, s_tenantid);
    
    time_t tnow = time (NULL);
    
    /* Figure out if we're renewing an existing session */
    session *S = session_find (netid, auth->sessionid);
    if (S) {
        /* Discard replays */
        if (S->lastserial < auth->serial) {
            log_debug ("Renewing session <%08x-%08x> from <%s> serial <%i> "
                       "tenant <%s> host <%s>", auth->sessionid, netid,
                      addrbuf, auth->serial, s_tenantid, s_hostid);
            S->key = auth->sessionkey;
            S->lastcycle = tnow;
            S->lastserial = auth->serial;
        }
        else {
            log_debug ("Rejecting Duplicate serial <%i> on auth from <%s>",
                        auth->serial, addrbuf);
        }
    }
    else {
        log_info ("New session <%08x-%08x> from <%s> serial <%i> "
                  "tenant <%s> host <%s>", auth->sessionid, netid,
                  addrbuf, auth->serial, s_tenantid, s_hostid);
    
        S = session_register (auth->tenantid, auth->hostid, netid,
                              auth->sessionid, auth->sessionkey,
                              remote);
        if (! S) log_error ("Session is NULL");
    }
    
    if (S) {
        host *h = S->host;
        char addrbuf[64];
        ip2str (&S->remote, addrbuf);
        meterid_t mid_agentip = makeid ("agent/ip", MTYPE_STR, 0);
        meter *m_agentip = host_get_meter (h, mid_agentip);
        meter_setcount (m_agentip, 0);
        meter_set_str (m_agentip, 0, addrbuf);
    }
    
    /* Now's a good time to cut the dead wood */
    session_expire (tnow - 905);
    var *vlist = sessionlist_save();
    db_set_global (APP.db, "sessions", vlist);
    var_free (vlist);
    free (auth);
}

/** Goes over fresh host metadata to pick up any parts relevant
  * to the operation of opticon-collector. At this moment,
  * this is just the meterwatch adjustments.
  * \param H The host we're dealing with
  * \param meta The fresh metadata
  */
void handle_host_metadata (host *H, var *meta) {
    /* layout of adjustment data:
       meter {
           pcpu {
               type: frac # int | frac | string
               warning { val: 30.0, weight: 1.0 }
               alert{ val: 50.0, weight: 1.0 }
           }
       }
    */
    
    char *tstr;
    var *v_meter = var_get_dict_forkey (meta, "meter");
    int adjcount = var_get_count (v_meter);
    if (adjcount) {
        var *v_adjust = v_meter->value.arr.first;
        while (v_adjust) {
            const char *levels[3] = {"warning","alert","critical"};
            watchadjusttype atype = WATCHADJUST_NONE;
            const char *stype = var_get_str_forkey (v_adjust, "type");
            if (! stype) break;
            if (memcmp (stype, "int", 3) == 0) atype = WATCHADJUST_UINT;
            else if (strcmp (stype, "frac") == 0) atype = WATCHADJUST_FRAC;
            else if (memcmp (stype, "str", 3) == 0) atype = WATCHADJUST_STR;
            meterid_t mid_adjust = makeid (v_adjust->id, 0, 0);
            watchadjust *adj = adjustlist_get (&H->adjust, mid_adjust);
            adj->type = atype;
            for (int i=0; i<3; ++i) {
                var *v_level = var_get_dict_forkey (v_adjust, levels[i]);
                switch (atype) {
                    case WATCHADJUST_UINT:
                        /* +1 needed to skip WATCHADJUST_NONE */
                        adj->adjust[i+1].data.u64 =
                            var_get_int_forkey (v_level, "val");
                        break;
                    
                    case WATCHADJUST_FRAC:
                        adj->adjust[i+1].data.frac =
                            var_get_double_forkey (v_level, "val");
                        break;
                    
                    case WATCHADJUST_STR:
                        tstr = adj->adjust[i+1].data.str.str;
                        strncpy (tstr,
                                 var_get_str_forkey(v_level, "val"), 127);
                        tstr[127] = 0;
                        break;
                    
                    default:
                        adj->adjust[i+1].weight = 0.0;
                        break;
                }
                if (atype != WATCHADJUST_NONE) {
                    adj->adjust[i+1].weight =
                        var_get_double_forkey (v_level, "weight");
                }
            }
            v_adjust = v_adjust->next;
        }
    }
}

/** Handler for a meter packet */
void handle_meter_packet (ioport *pktbuf, uint32_t netid) {
    session *S = NULL;
    ioport *unwrap;
    time_t tnow = time (NULL);
    
    /* Unwrap the outer packet layer (crypto and compression) */
    unwrap = ioport_unwrap_meterdata (netid, pktbuf,
                                      resolve_sessionkey, (void**) &S);
    if (! unwrap) {
        log_debug ("Error unwrapping packet from <%08x>: %08x", netid, unwrap_errno);
        return;
    }
    if (! S) {
        log_debug ("Error unwrapping session from <%08x>", netid);
        ioport_close (unwrap);
        return;
    }
    
    /* Write the new meterdata into the host */
    host *H = S->host;
    pthread_rwlock_wrlock (&H->lock);
    
    /* Check if we need to sync up metadata */
    if (tnow - H->lastmetasync > 300) {
        if (! db_open (APP.db, S->tenantid, NULL)) {
            log_error ("Could not load tenant for already open session");
        }
        else {
            time_t changed = db_get_hostmeta_changed (APP.db, S->hostid);
            if (changed > H->lastmetasync) {
                log_debug ("Reloading metadata");
                var *meta = db_get_hostmeta (APP.db, S->hostid);
                handle_host_metadata (H, meta);
                var_free (meta);
            }
            db_close (APP.db);
        }
        H->lastmetasync = tnow;
    }
    
    host_begin_update (H, tnow);
    if (codec_decode_host (APP.codec, unwrap, H)) {
        log_debug ("Update handled for session <%08x-%08x>",
                    S->sessid, S->addr);
    }
    
    host_end_update (H);
    pthread_rwlock_unlock (&H->lock);
    ioport_close (unwrap);
}

/** Thread runner for handling configuration reload requests.
  * This is done to keep the signal handler path clean. */
void conf_reloader_run (thread *t) {
    conf_reloader *self = (conf_reloader *) t;
    while (1) {
        conditional_wait_fresh (self->cond);
        if (! load_json (APP.conf, APP.confpath)) {
            log_error ("Error loading %s: %s\n",
                       APP.confpath, parse_error());
        }
        else {
            opticonf_handle_config (APP.conf);
        }
    }
}

/** Create the configuration reloader thread */
conf_reloader *conf_reloader_create (void) {
    conf_reloader *self = (conf_reloader *) malloc (sizeof (conf_reloader));
    self->cond = conditional_create();
    thread_init (&self->super, conf_reloader_run, NULL);
    return self;
}

/** Signal the configuration reloader thread to do a little jig */
void conf_reloader_reload (conf_reloader *self) {
    conditional_signal (self->cond);
}

/** Signal handler for SIGHUP */
void daemon_sighup_handler (int sig) {
    conf_reloader_reload (APP.reloader);
    signal (SIGHUP, daemon_sighup_handler);
}

/** Main loop. Waits for a packet, then handles it. */
int daemon_main (int argc, const char *argv[]) {
    if (strcmp (APP.logpath, "@syslog") == 0) {
        log_open_syslog ("opticon-collector", APP.loglevel);
    }
    else {
        log_open_file (APP.logpath, APP.loglevel);
    }

    log_info ("--- Opticon-collector ready for action ---");
    
    var *slist = db_get_global (APP.db, "sessions");
    if (slist) {
        log_info ("Restoring sessions");
        sessionlist_restore (slist);
        var_free (slist);
        session_expire (time(NULL) - 905);
    }
    
    
    /* Set up threads */
    APP.queue = packetqueue_create (1024, APP.transport);
    APP.reloader = conf_reloader_create();
    APP.watchthread = thread_create (watchthread_run, NULL);
    
    signal (SIGHUP, daemon_sighup_handler);
    
    while (1) {
        pktbuf *pkt = packetqueue_waitpkt (APP.queue);
        if (pkt) {
            uint32_t netid = gen_networkid (&pkt->addr);
            ioport *pktbufport = ioport_create_buffer (pkt->pkt, 2048);
            /* Pointing it at itself will just move the cursor */
            ioport_write (pktbufport, pkt->pkt, pkt->sz);
            
            /* Check packet version info */
            if (pkt->pkt[0] == 'o' && pkt->pkt[1] == '6') {
                if (pkt->pkt[2] == 'a' && pkt->pkt[3] == '1') {
                    /* AUTH v1 */
                    handle_auth_packet (pktbufport, netid, &pkt->addr);
                }
                else if (pkt->pkt[2] == 'm' && pkt->pkt[3] == '1') {
                    /* METER v1 */
                    handle_meter_packet (pktbufport, netid);
                }
            }
            ioport_close (pktbufport);
            pkt->pkt[0] = 0;
        }
    }
    return 0;
}

/** Set up foreground flag */
int set_foreground (const char *i, const char *v) {
    APP.foreground = 1;
    return 1;
}

/** Set up configuration file path */
int set_confpath (const char *i, const char *v) {
    APP.confpath = v;
    return 1;
}

int set_mconfpath (const char *i, const char *v) {
    APP.mconfpath = v;
    return 1;
}

/** Set up pidfile path */
int set_pidfile (const char *i, const char *v) {
    APP.pidfile = v;
    return 1;
}

/** Set the logfile path, @syslog for logging to syslog */
int set_logpath (const char *i, const char *v) {
    APP.logpath = v;
    return 1;
}

/** Handle --loglevel */
int set_loglevel (const char *i, const char *v) {
    if (strcmp (v, "CRIT") == 0) APP.loglevel = LOG_CRIT;
    else if (strcmp (v, "ERR") == 0) APP.loglevel = LOG_ERR;
    else if (strcmp (v, "WARNING") == 0) APP.loglevel = LOG_WARNING;
    else if (strcmp (v, "INFO") == 0) APP.loglevel = LOG_INFO;
    else if (strcmp (v, "DEBUG") == 0) APP.loglevel = LOG_DEBUG;
    else APP.loglevel = LOG_WARNING;
    return 1;
}

/** Set up network configuration */
int conf_network (const char *id, var *v, updatetype tp) {
    switch (tp) {
        case UPDATE_CHANGE:
        case UPDATE_REMOVE:
            /* Do the lame thing, respawn from the watchdog with the
               new settings */
            exit (0);
        case UPDATE_ADD:
            APP.listenport = var_get_int_forkey (v, "port");
            APP.listenaddr = var_get_str_forkey (v, "address");
            log_info ("Listening on %s:%i", APP.listenaddr, APP.listenport);
            if (strcmp (APP.listenaddr, "*") == 0) {
                APP.listenaddr = "::";
            }
            break;
        case UPDATE_NONE:
            break;
    }
    return 1;
}

/** Set up database path from configuration */
int conf_db_path (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) return 0;
    if (tp == UPDATE_CHANGE) {
        db_free (APP.db);
        db_free (APP.writedb);
    }
    APP.db = localdb_create (var_get_str (v));
    APP.writedb = localdb_create (var_get_str (v));
    return 1;
}

/** Set up watchlist from meter definitions */
int conf_meters (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) return 0;
    watchlist_populate (&APP.watch, v);
    return 1;
}

appcontext APP; /**< Global application context */

/** Command line options */
cliopt CLIOPT[] = {
    {"--foreground","-f",OPT_FLAG,NULL,set_foreground},
    {"--pidfile","-p",OPT_VALUE,
        "/var/run/opticon-collector.pid", set_pidfile},
    {"--logfile","-l",OPT_VALUE, "@syslog", set_logpath},
    {"--loglevel","-L",OPT_VALUE, "INFO", set_loglevel},
    {"--config-path","-c",OPT_VALUE,
        "/etc/opticon/opticon-collector.conf", set_confpath},
    {"--meter-config-path","-m",OPT_VALUE,
        "/etc/opticon/opticon-meter.conf", set_mconfpath},
    {NULL,NULL,0,NULL,NULL}
};

/** Application main. Handles configuration and command line, sets up
  * the application context APP and spawns daemon_main().
  */
int main (int _argc, const char *_argv[]) {
    int argc = _argc;
    const char **argv = cliopt_dispatch (CLIOPT, _argv, &argc);
    if (! argv) return 1;

    opticonf_add_reaction ("network", conf_network);
    opticonf_add_reaction ("database/path", conf_db_path);
    opticonf_add_reaction ("meter", conf_meters);
    
    APP.transport = intransport_create_udp();
    APP.codec = codec_create_pkt();

    APP.conf = var_alloc();
    
    /* Preload the default meter set */
    var *defmeters = get_default_meterdef();
    sprintf (defmeters->id, "meter");
    var_link (defmeters, APP.conf);
    
    /* Load other meters from meter.conf */
    if (! load_json (defmeters, APP.mconfpath)) {
        log_error ("Error loading %s: %s\n",
                   APP.confpath, parse_error());
        return 1;
    }
    
    /* Now load the main config in the same var space */
    if (! load_json (APP.conf, APP.confpath)) {
        log_error ("Error loading %s: %s\n",
                   APP.confpath, parse_error());
        return 1;
    }
    
    opticonf_handle_config (APP.conf);
    
    log_info ("Configuration loaded");
    
    if (! intransport_setlistenport (APP.transport, APP.listenaddr, 
                                     APP.listenport)) {
        log_error ("Error setting listening port");
    }
    if (! daemonize (APP.pidfile, argc, argv, daemon_main, APP.foreground)) {
        log_error ("Error spawning");
        return 1;
    }
    return 0;
}
