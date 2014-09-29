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
#include <libopticon/import.h>
#include <arpa/inet.h>
#include <syslog.h>

appcontext APP;

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
            log_debug ("Authenticating session <%08x>", sid);
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
                        var_to_host (h, (var *) v);
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
    
    if (! daemonize (APP.pidfile, argc, argv, daemon_main, APP.foreground)) {
        log_error ("Error spawning");
        return 1;
    }
    
    return 0;
}
