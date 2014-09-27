#include "opticon-agent.h"
#include <libopticon/ioport.h>
#include <libopticon/codec_pkt.h>
#include <libopticon/pktwrap.h>
#include <libopticon/util.h>
#include <libopticon/var.h>
#include <libopticon/parse.h>
#include <libopticon/react.h>
#include <libopticon/daemon.h>
#include <libopticon/log.h>
#include <libopticon/cliopt.h>
#include <libopticon/transport_udp.h>
#include <arpa/inet.h>
#include <syslog.h>

appcontext APP;

/** Changes an array of dictionaries from var data into a meter-style
  * 'dictionary of arrays'
  * \param h The host to write meters to.
  * \param prefix The name of the enveloping array.
  * \param v The first array node.
  */
void dictarray_to_host (host *h, const char *prefix, var *v) {
    char tmpid[16];
    meterid_t mid;
    meter *m;
    
    var *first = v;
    var *vv = first->value.arr.first;
    var *crsr;
    while (vv) {
        const char *curid = vv->id;
        if (strlen (prefix) + strlen (curid) > 10) {
            log_error ("Error parsing meter result, label '%s/%s' too "
                       "long", prefix, curid);
            return;
        }
        sprintf (tmpid, "%s/%s", prefix, curid);

        crsr = v;
        metertype_t type;
        
        switch (vv->type) {
            case VAR_INT:
                type = MTYPE_INT;
                break;
            
            case VAR_DOUBLE:
                type = MTYPE_FRAC;
                break;
            
            case VAR_STR:
                type = MTYPE_STR;
                break;
            
            default:
                log_error ("Unsupported subtype %i under '%s'", vv->type, tmpid);
                return;
        }

        mid = makeid (tmpid, type, 0);
        m = host_get_meter (h, mid);

        int pos = 0;
        int count = v->parent->value.arr.count;
        meter_setcount (m, count);
        
        while (crsr && (pos < count)) {
            if (type == MTYPE_INT) {
                meter_set_uint (m, pos, var_get_int_forkey (crsr, vv->id));
            }
            else if (type == MTYPE_STR) {
                meter_set_str (m, pos, var_get_str_forkey (crsr, vv->id));
            }
            else {
                meter_set_frac (m, pos, var_get_double_forkey (crsr, vv->id));
            }
            pos++;
            crsr = crsr->next;
        }
        vv = vv->next;
    }
}

/** Pick up JSON results from a probe and turn them into metering
  * data.
  * \param h The host to write meters to
  * \param val The data to parse.
  */
void result_to_host (host *h, volatile var *val) {
    var *v = val->value.arr.first;
    int count;
    char firstlevel[16];
    char tmpid[16];
    while (v) {
        if (strlen (v->id) > 11) {
            log_error ("Error parsing meter result, label '%s' too long",
                       v->id);
            break;
        }
        meterid_t mid;
        meter *m;
        if (v->type == VAR_DOUBLE) {
            mid = makeid (v->id, MTYPE_FRAC, 0);
            m = host_get_meter (h, mid);
            meter_setcount (m, 0);
            meter_set_frac (m, 0, var_get_double (v));
        }
        else if (v->type == VAR_INT) {
            mid = makeid (v->id, MTYPE_INT, 0);
            m = host_get_meter (h, mid);
            meter_setcount (m, 0);
            meter_set_uint (m, 0, var_get_int (v));
        }
        else if (v->type == VAR_STR) {
            mid = makeid (v->id, MTYPE_STR, 0);
            m = host_get_meter (h, mid);
            meter_setcount (m, 0);
            meter_set_str (m, 0, var_get_str (v));
        }
        else if (v->type == VAR_ARRAY) {
            strcpy (firstlevel, v->id);
            var *vv = v->value.arr.first;
            if (vv) {
                if (vv->type == VAR_ARRAY) {
                    log_error ("Error parsing meter result, nested "
                               "array under '%s' not supported",
                               firstlevel);
                    break;
                }
                if (vv->type == VAR_DICT) {
                    dictarray_to_host (h, firstlevel, vv);
                }
                else if (vv->type == VAR_DOUBLE) {
                    count = v->value.arr.count;
                    mid = makeid (v->id, MTYPE_FRAC,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, count);
                    for (int i=0; i<count; ++i) {
                        double d = var_get_double_atindex (v, i);
                        meter_set_frac (m, i, d);
                    }
                }
                else if (vv->type == VAR_INT) {
                    count = v->value.arr.count;
                    mid = makeid (v->id, MTYPE_INT,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, count);
                    for (int i=0; i<count; ++i) {
                        uint64_t d = (uint64_t) var_get_int_atindex (v, i);
                        meter_set_uint (m, i, d);
                    }
                }
                else if (vv->type == VAR_STR) {
                    count = v->value.arr.count;
                    mid = makeid (v->id, MTYPE_STR,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, count);
                    for (int i=0; i<count; ++i) {
                        meter_set_str (m, i, var_get_str_atindex (v,i));
                    }
                }
            }
        }
        else if (v->type == VAR_DICT) {
            strcpy (firstlevel, v->id);
            var *vv = v->value.arr.first;
            while (vv) {
                if (strlen (vv->id) + strlen (firstlevel) > 10) {
                    log_error ("Error parsing meter result, label "
                               "'%s/%s' too long", firstlevel, v->id);
                    break;
                }
                sprintf (tmpid, "%s/%s", firstlevel, vv->id);
                if (vv->type == VAR_ARRAY) {
                    log_error ("Error parsing meter result, array "
                               "under dict '%s' not supported",
                               firstlevel);
                    break;
                }
                if (vv->type == VAR_ARRAY) {
                    log_error ("Error parsing meter result, nested "
                               "dict under '%s' not supported",
                               firstlevel);
                    break;
                }
                if (vv->type == VAR_DOUBLE) {
                    mid = makeid (tmpid, MTYPE_FRAC,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, 0);
                    meter_set_frac (m, 0, var_get_double (vv));
                }
                else if (vv->type == VAR_INT) {
                    mid = makeid (tmpid, MTYPE_INT,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, 0);
                    meter_set_uint (m, 0, var_get_int (vv));
                }
                else if (vv->type == VAR_STR) {
                    mid = makeid (tmpid, MTYPE_STR,0);
                    m = host_get_meter (h, mid);
                    meter_setcount (m, 0);
                    meter_set_str (m, 0, var_get_str (vv));
                }
                vv = vv->next;
            }
        }
        
        v = v->next;
    }
}

void __breakme (void) {}

/** Daemon main run loop. */
int daemon_main (int argc, const char *argv[]) {
    const char *buf;
    size_t sz;

    if (strcmp (APP.logpath, "@syslog") == 0) {
        log_open_syslog ("opticon-collector", APP.loglevel);
    }
    else {
        log_open_file (APP.logpath, APP.loglevel);
    }
    
    probelist_start (&APP.probes);
    APP.resender = authresender_create (APP.transport);
    
    time_t tlast = time (NULL);
    time_t nextslow = tlast + 5;
    time_t nextsend = tlast + 10;
    time_t lastkeyrotate = 0;
    int slowround = 0;

    log_info ("Daemonized");
    while (1) {
        time_t tnow = tlast = time (NULL);
        
        slowround = 0;
        
        /* If a slow round is due at this time, use the excuse to send an
           authentication packet */
        if (nextslow <= tnow) {
            slowround = 1;
            uint32_t sid = APP.auth.sessionid;
            if (! sid) sid = gen_sessionid();
            log_debug ("Authenticating session %08x", sid);
            APP.auth.sessionid = sid;
            APP.auth.serial = 0;
            APP.auth.tenantid = APP.tenantid;
            APP.auth.hostid = APP.hostid;
            
            /* Only rotate the AES key every half hour */
            if (tnow - lastkeyrotate > 1800) {
                APP.auth.sessionkey = aeskey_create();
                lastkeyrotate = tnow;
            }
            APP.auth.tenantkey = APP.collectorkey;
            
            /* Dispatch */
            ioport *io_authpkt = ioport_wrap_authdata (&APP.auth,
                                                       gen_serial());
            
            sz = ioport_read_available (io_authpkt);
            buf = ioport_get_buffer (io_authpkt);
            outtransport_send (APP.transport, (void*) buf, sz);
            authresender_schedule (APP.resender, buf, sz);
            ioport_close (io_authpkt);
            
            /* Schedule next slow round */
            nextslow = nextslow + 300;
        }
        
        log_debug ("Poking probes");

        probe *p = APP.probes.first;
        time_t wakenext = tnow + 300;

        /* Go over the probes to figure out whether we should kick them */        
        while (p) {
            time_t firewhen = p->lastpulse + p->interval;
            if (firewhen <= tnow) {
                conditional_signal (&p->pulse);
                p->lastpulse = tnow;
            }
            
            /* Figure out whether the next event for this probe is sooner
               than the next wake-up time we determined so far */
            if (p->lastpulse + p->interval < wakenext) {
                wakenext = p->lastpulse + p->interval;
            }
            
            p = p->next;
        }
        
        int collected = 0;
        host *h = host_alloc();
        
        /* If we're in a slow round, we already know we're scheduled. Otherwise,
           see if the next scheduled moment for sending a (fast lane) packet
           has passed. */
        if (slowround || (tnow >= nextsend)) {
            h->uuid = APP.hostid;
            host_begin_update (h, time (NULL));

            if (! slowround) while (nextsend <= tnow) nextsend += 60;
            log_debug ("Collecting probes");
        
            /* Go over the probes again, picking up the one relevant to the
               current round being performed */
            p = APP.probes.first;
            while (p) {
                volatile var *v = p->vcurrent;
                /* See if data for this probe has been collected since the last kick */
                if (v && (p->lastdispatch < p->lastreply)) {
                    /* Filter probes for the current lane */
                    if ((slowround && p->interval>60) ||
                        ((!slowround) && p->interval<61)) {
                        log_debug ("Collecting '%s'", p->call);
                        result_to_host (h, v);
                        p->lastdispatch = tnow;
                        collected++;
                    }
                }
                p = p->next;
            }
        }
        
        /* If any data was collected, encode it */
        if (collected) {
            log_debug ("Encoding probes");
        
            ioport *encoded = ioport_create_buffer (NULL, 4096);
            if (! encoded) {
                log_warn ("Error creating ioport");
                ioport_close (encoded);
                host_delete (h);
                continue;
            }
        
            if (! codec_encode_host (APP.codec, encoded, h)) {
                log_warn ("Error encoding host");
                ioport_close (encoded);
                host_delete (h);
                continue;
            }
        
            log_debug ("Encoded %i bytes", ioport_read_available (encoded));

            ioport *wrapped = ioport_wrap_meterdata (APP.auth.sessionid,
                                                     gen_serial(),
                                                     APP.auth.sessionkey,
                                                     encoded);
        
        
            if (! wrapped) {
                log_error ("Error wrapping");
                ioport_close (encoded);
                host_delete (h);
                continue;
            }
        
            sz = ioport_read_available (wrapped);
            buf = ioport_get_buffer (wrapped);
        
            /* Send it off into space */
            outtransport_send (APP.transport, (void*) buf, sz);
            log_info ("%s lane packet sent: %i bytes", 
                      slowround ? "Slow":"Fast", sz);

            ioport_close (wrapped);
            ioport_close (encoded);
            host_delete (h);
        }
        
        /* Figure out what the next scheduled wake-up time is */
        tnow = time (NULL);
        if (nextsend < wakenext) wakenext = nextsend;
        if (nextslow < wakenext) wakenext = nextslow;
        if (wakenext > tnow) {
            log_debug ("Sleeping for %i seconds", (wakenext-tnow));
            sleep (wakenext-tnow);
        }
    }
    return 666;
}

/** Parse /collector/address */
int conf_collector_address (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) exit (0);
    APP.collectoraddr = strdup (var_get_str (v));
    return 1;
}

/** Parse /collector/tenant */
int conf_tenant (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) exit (0);
    APP.tenantid = mkuuid (var_get_str (v));
    return 1;
}

/** Parse /collector/host */
int conf_host (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) exit (0);
    APP.hostid = mkuuid (var_get_str (v));
    return 1;
}

/** Parse /collector/port */
int conf_collector_port (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) exit (0);
    APP.collectorport = var_get_int (v);
    return 1;
}

/** Parse /collector/key */
int conf_collector_key (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) exit (0);
    APP.collectorkey = aeskey_from_base64 (var_get_str (v));
    return 1;
}

/** Parse /meter into probes */
int conf_probe (const char *id, var *v, updatetype tp) {
    if (tp != UPDATE_ADD) exit (0);
    const char *vtp = var_get_str_forkey (v, "type");
    const char *call = var_get_str_forkey (v, "call");
    int interval = var_get_int_forkey (v, "interval");
    probetype t = PROBE_BUILTIN;
    
    if (vtp && call && interval) {
        if (strcmp (vtp, "exec") == 0) t = PROBE_EXEC;
        if (probelist_add (&APP.probes, t, call, interval)) {
            return 1;
        }
        else {
            log_error ("Error adding probe-call '%s'", call);
        }
    }
    return 0;
}

/** Handle --foreground */
int set_foreground (const char *i, const char *v) {
    APP.foreground = 1;
    return 1;
}

/** Handle --config-path */
int set_confpath (const char *i, const char *v) {
    APP.confpath = v;
    return 1;
}

/** Handle --pidfile */
int set_pidfile (const char *i, const char *v) {
    APP.pidfile = v;
    return 1;
}

/** Handle --logfile */
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

/** Command line options */
cliopt CLIOPT[] = {
    {
        "--foreground",
        "-f",
        OPT_FLAG,
        NULL,
        set_foreground
    },
    {
        "--pidfile",
        "-p",
        OPT_VALUE,
        "/var/run/opticon-agent.pid",
        set_pidfile
    },
    {
        "--logfile",
        "-l",
        OPT_VALUE,
        "@syslog",
        set_logpath
    },
    {
        "--loglevel",
        "-L",
        OPT_VALUE,
        "INFO",
        set_loglevel
    },
    {
        "--config-path",
        "-c",
        OPT_VALUE,
        "/etc/opticon/opticon-collector.conf",
        set_confpath
    },
    {NULL,NULL,0,NULL,NULL}
};

/** Application main. Handles configuration and command line flags,
  * then daemonizes.
  */
int main (int _argc, const char *_argv[]) {
    int argc = _argc;
    const char **argv = cliopt_dispatch (CLIOPT, _argv, &argc);
    if (! argv) return 1;
    
    opticonf_add_reaction ("collector/address", conf_collector_address);
    opticonf_add_reaction ("collector/port", conf_collector_port);
    opticonf_add_reaction ("collector/key", conf_collector_key);
    opticonf_add_reaction ("collector/tenant", conf_tenant);
    opticonf_add_reaction ("collector/host", conf_host);
    opticonf_add_reaction ("probes/*", conf_probe);
    
    APP.transport = outtransport_create_udp();
    APP.codec = codec_create_pkt();
    APP.conf = var_alloc();

    if (! load_json (APP.conf, APP.confpath)) {
        log_error ("Error loading %s: %s\n", APP.confpath, parse_error());
        return 1;
    }
    
    opticonf_handle_config (APP.conf);
    
    if (! outtransport_setremote (APP.transport, APP.collectoraddr,
                                  APP.collectorport)) {
        log_error ("Error setting remote address");
        return 1;
    }
    
    if (! daemonize (APP.pidfile, argc, argv, daemon_main)) {
        log_error ("Error spawning");
        return 1;
    }
    
    return 0;
}
