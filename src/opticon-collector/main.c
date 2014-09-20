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

/** Looks up a session by netid and sessionid. If it's a valid session,
  * returns its current AES session key.
  * \param netid The host's netid (32 bit ip-derived).
  * \param sid The session-id.
  * \param serial Serial-number of packet being handled.
  * \param blob Pointer for giving back the session.
  * \return The key, or NULL if resolving failed, or session was invalid.
  */
aeskey *resolve_sessionkey (uint32_t netid, uint32_t sid, uint32_t serial,
                            void **blob) {
    session *S = session_find (netid, sid);
    if (! S) return NULL;
    if (S->lastserial >= serial) return NULL;
    S->lastserial = serial;
    *blob = (void *)S;
    return &S->key;
}

/** Looks up a tenant in memory and in the database, does the necessary
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
    
    /* Discard replays */
    if (T && T->lastserial >= serial) return NULL;
    
    /* Discard tenants we can't find in the database */
    if (! db_open (APP.db, tenantid, NULL)) {
        // FIXME remove tenant
        return NULL;
    }
    
    /* Discard tenants with no metadata */
    if (! (meta = db_get_metadata (APP.db))) {
        db_close (APP.db);
        return NULL;
    }
    
    /* Find the 'key' metavalue */
    if ((b64 = var_get_str_forkey (meta, "key"))) {
        if (T) {
            /* Update the existing tenant structure with information
               from the database */
            T->lastserial = serial;
            T->key = aeskey_from_base64 (b64);
            res = &T->key;
        }
        else {
            /* Create a new in-memory tenant */
            T = tenant_create (tenantid, aeskey_from_base64 (b64));
            if (T) {
                T->lastserial = serial;
                res = &T->key;
            }
        }
    }
    db_close (APP.db);
    var_free (meta);
    return res;
}

/** Handler for an auth packet */
void handle_auth_packet (ioport *pktbuf, uint32_t netid) {
    authinfo *auth = ioport_unwrap_authdata (pktbuf, resolve_tenantkey);
    
    /* No auth means discarded by the crypto/decoding layer */
    if (! auth) return;
    time_t tnow = time (NULL);
    
    /* Figure out if we're renweing an existing session */
    session *S = session_find (netid, auth->sessionid);
    if (S) {
        /* Discard replays */
        if (S->lastserial >= auth->serial) {
            S->key = auth->sessionkey;
            S->lastcycle = tnow;
            S->lastserial = auth->serial;
        }
    }
    else {
        S = session_register (auth->tenantid, auth->hostid, netid,
                              auth->sessionid, auth->sessionkey);
    }
    
    /* Now's a good time to cut the dead wood */
    session_expire (tnow - 600);
    free (auth);
}

/** Handler for a meter packet */
void handle_meter_packet (ioport *pktbuf, uint32_t netid) {
    session *S = NULL;
    ioport *unwrap;
    
    /* Unwrap the outer packet layer (crypto and compression) */
    unwrap = ioport_unwrap_meterdata (netid, pktbuf,
                                      resolve_sessionkey, (void**) &S);
    if (! unwrap) return;
    if (! S) {
        ioport_close (unwrap);
        return;
    }
    
    /* Write the new meterdata into the host */
    host *H = S->host;
    if (codec_decode_host (APP.codec, unwrap, H)) {
        if (db_open (APP.db, S->tenantid, NULL)) {
            db_save_record (APP.db, time(NULL), H);
            db_close (APP.db);
        }
    }
    ioport_close (unwrap);
}

/** Main loop. Waits for a packet, then handles it. */
int daemon_main (int argc, const char *argv[]) {
    log_open_syslog ("opticon-collector");
    log_info ("Daemonized\n");
    while (1) {
        pktbuf *pkt = packetqueue_waitpkt (APP.queue);
        if (pkt) {
            uint32_t netid = gen_networkid (&pkt->addr);
            ioport *pktbuf = ioport_create_buffer (pkt->pkt, 2048);
            /* Pointing it at itself will just move the cursor */
            ioport_write (pktbuf, pkt->pkt, pkt->sz);
            
            /* Check packet version info */
            if (pkt->pkt[0] == 'o' && pkt->pkt[1] == '6') {
                if (pkt->pkt[2] == 'a' && pkt->pkt[3] == '1') {
                    /* AUTH v1 */
                    handle_auth_packet (pktbuf, netid);
                }
                else if (pkt->pkt[2] == 'm' && pkt->pkt[3] == '1') {
                    /* METER v1 */
                    handle_meter_packet (pktbuf, netid);
                }
            }
            ioport_close (pktbuf);
        }
        log_info ("Starting round\n");
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

/** Set up pidfile path */
int set_pidfile (const char *i, const char *v) {
    APP.pidfile = v;
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
        db_close (APP.db);
        db_free (APP.db);
    }
    APP.db = localdb_create (var_get_str (v));
    return 1;
}

appcontext APP; /**< Global application context */

/** Command line options */
cliopt CLIOPT[] = {
    {"--foreground","-f",OPT_FLAG,NULL,set_foreground},
    {"--pidfile","-p",OPT_VALUE,
        "/var/run/opticon-collector.pid", set_pidfile},
    {"--config-path","-c",OPT_VALUE,
        "/etc/opticon/opticon-collector.conf", set_confpath},
    {NULL,NULL,0,NULL,NULL}
};

/** Application main. Handles configuration and command line, sets up
  * the application context APP and spawns daemon_main().
  */
int main (int _argc, const char *_argv[]) {
    int argc = _argc;
    const char **argv = cliopt_dispatch (CLIOPT, _argv, &argc);
    if (! argv) return 1;

    opticonf_add_reaction ("network/port", conf_network);
    opticonf_add_reaction ("network/address", conf_network);
    opticonf_add_reaction ("database/path", conf_db_path);
    
    APP.transport = intransport_create_udp();
    APP.codec = codec_create_pkt();

    APP.conf = var_alloc();
    if (! load_json (APP.conf, APP.confpath)) {
        log_error ("Error loading %s: %s\n",
                 APP.confpath, parse_error());
        return 1;
    }
    
    log_info ("Configuration loaded");
    intransport_setlistenport (APP.transport, APP.listenaddr, APP.listenport);
    APP.queue = packetqueue_create (1024, APP.transport);
    if (! daemonize (APP.pidfile, argc, argv, daemon_main)) {
        log_error ("Error spawning");
        return 1;
    }
    return 0;
}
