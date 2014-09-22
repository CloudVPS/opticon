#include "opticon-agent.h"
#include <libopticon/auth.h>
#include <libopticon/ioport.h>
#include <libopticon/codec_pkt.h>
#include <libopticon/pktwrap.h>
#include <libopticon/util.h>
#include <libopticonf/var.h>
#include <libopticonf/parse.h>
#include <libopticonf/react.h>
#include <libsvc/daemon.h>
#include <libsvc/log.h>
#include <libsvc/cliopt.h>
#include <libsvc/transport_udp.h>
#include <arpa/inet.h>

appcontext APP;

int daemon_main (int argc, const char *argv[]) {
    if (strcmp (APP.logpath, "@syslog") == 0) {
        log_open_syslog ("opticon-collector");
    }
    else {
        log_open_file (APP.logpath);
    }

    log_info ("Daemonized");
    while (1) sleep (60);
    return 666;
}

int conf_collector_address (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) exit (0);
    APP.collectoraddr = strdup (var_get_str (v));
    return 1;
}

int conf_tenant (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) exit (0);
    APP.tenantid = mkuuid (var_get_str (v));
    return 1;
}

int conf_host (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) exit (0);
    APP.hostid = mkuuid (var_get_str (v));
    return 1;
}

int conf_collector_port (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) exit (0);
    APP.collectorport = var_get_int (v);
    return 1;
}

int conf_collector_key (const char *id, var *v, updatetype tp) {
    if (tp == UPDATE_REMOVE) exit (0);
    APP.collectorkey = aeskey_from_base64 (var_get_str (v));
    return 1;
}

int conf_meter (const char *id, var *v, updatetype tp) {
    if (tp != UPDATE_ADD) exit (0);
    const char *vtp = var_get_str_forkey (v, "type");
    const char *call = var_get_str_forkey (v, "call");
    int interval = var_get_int_forkey (v, "interval");
    probetype t = PROBE_BUILTIN;
    
    if (vtp && call && interval) {
        if (strcmp (vtp, "exec") == 0) t = PROBE_EXEC;
        probelist_add (&APP.probes, t, call, interval);
        return 1;
    }
    return 0;
}

int set_foreground (const char *i, const char *v) {
    APP.foreground = 1;
    return 1;
}

int set_confpath (const char *i, const char *v) {
    APP.confpath = v;
    return 1;
}

int set_pidfile (const char *i, const char *v) {
    APP.pidfile = v;
    return 1;
}

int set_logpath (const char *i, const char *v) {
    APP.logpath = v;
    return 1;
}

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
        "--config-path",
        "-c",
        OPT_VALUE,
        "/etc/opticon/opticon-collector.conf",
        set_confpath
    }
};

int main (int _argc, const char *_argv[]) {
    int argc = _argc;
    const char **argv = cliopt_dispatch (CLIOPT, _argv, &argc);
    if (! argv) return 1;
    
    opticonf_add_reaction ("collector/address", conf_collector_address);
    opticonf_add_reaction ("collector/port", conf_collector_port);
    opticonf_add_reaction ("collector/key", conf_collector_key);
    opticonf_add_reaction ("collector/tenant", conf_tenant);
    opticonf_add_reaction ("collector/host", conf_host);
    opticonf_add_reaction ("meter/*", conf_meter);
    
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
