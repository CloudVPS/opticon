#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libopticondb/db_local.h>
#include <libsvc/cliopt.h>

#include "cmd.h"

optinfo OPTIONS = { "","","","",0 };

/** Handle --tenant */
int set_tenant (const char *o, const char *v) { 
    OPTIONS.tenant = v;
    return 1;
}

/** Handle --key */
int set_key (const char *o, const char *v) {
    OPTIONS.key = v;
    return 1;
}

/** Handle --path */
int set_path (const char *o, const char *v) {
    OPTIONS.path = v;
    return 1;
}

/** Handle --host */
int set_host (const char *o, const char *v) {
    OPTIONS.host = v;
    return 1;
}

/** Handle --json */
int set_json (const char *o, const char *v) {
    OPTIONS.json = 1;
    return 1;
}

/** Handle --time */
int set_time (const char *o, const char *v) {
    time_t tnow = time (NULL);
    
    if (strcmp (v, "now") == 0) {
        OPTIONS.time = tnow;
        return 1;
    }
    
    struct tm tm;
    int year,month,day,hour,minute;
    
    /* HH:MM */
    if (strlen (v) == 5 && v[2] == ':') {
        localtime_r (&tnow, &tm);
        hour = atoi (v);
        minute = atoi (v+3);
        if (hour<0 || hour>23 || minute < 0 || minute > 60) {
            fprintf (stderr, "%% Illegal timespec: %s\n", v);
            return 0;
        }
        if (hour < tm.tm_hour) {
            tm.tm_hour = hour;
            tm.tm_min = minute;
        }
        else if (hour == tm.tm_hour && minute < tm.tm_min) {
            tm.tm_min = minute;
        }
        else {
            tnow = time (NULL) - 86400;
            localtime_r (&tnow, &tm);
            tm.tm_hour = hour;
            tm.tm_min = minute;
        }
        OPTIONS.time = mktime (&tm);
        return 1;
    }
    
    /* yyyy-mm-ddThh:mm */
    if (strlen (v) == 16 && v[4] == '-' && v[7] == '-' &&
        v[10] == 'H' && v[13] == ':') {
        tm.tm_year = atoi (v) - 1900;
        tm.tm_mon = atoi (v+5) - 1;
        tm.tm_mday = atoi (v+8);
        tm.tm_hour = atoi (v+11);
        tm.tm_min = atoi (v+14);
        tm.tm_sec = 0;
        OPTIONS.time = mktime (&tm);
        return 1;
    }
    
    if (v[0]=='-' && atoi (v+1)) {
        char period = v[strlen(v)-1];
        switch (period) {
            case 'd':
                tnow = tnow - (86400 * atoi (v+1));
                break;
            
            case 'h':
                tnow = tnow - (3600 * atoi (v+1));
                break;
            
            case 'm':
                tnow = tnow - (60 * atoi (v+1));
                break;
            
            default:
                fprintf (stderr, "%% Illegal timespec: %s\n", v);
                return 0;
        }
        OPTIONS.time = tnow;
        return 1;
    }
    return 0;
}

/** Command line flags */
cliopt CLIOPT[] = {
    {"--tenant","-t",OPT_VALUE,"",set_tenant},
    {"--key","-k",OPT_VALUE,"",set_key},
    {"--host","-h",OPT_VALUE,"",set_host},
    {"--time","-T",OPT_VALUE,"now",set_time},
    {"--path","-p",OPT_VALUE,"/var/opticon/db",set_path},
    {"--json","-j",OPT_FLAG,NULL,set_json},
    {NULL,NULL,0,NULL,NULL}
};

/** Sub-commands */
clicmd CLICMD[] = {
    {"tenant-list",cmd_tenant_list},
    {"tenant-create",cmd_tenant_create},
    {"tenant-delete",cmd_tenant_delete},
    {"tenant-get-metadata",cmd_tenant_get_metadata},
    {"host-list",cmd_host_list},
    {"add-record",cmd_add_record},
    {"get-record",cmd_get_record},
    {NULL,NULL}
};

/** Print usage information.
  * \param cmdname argv[0]
  */
void usage (const char *cmdname) {
    fprintf (stderr,
        "%% Usage: %s <command> [options]\n"
         "  Options:\n"
         "        --path <path to database>\n"
         "        --json\n"
         "\n"
         "  Commands:\n"
         "        tenant-list\n"
         "        tenant-get-metadata --tenant <uuid>\n"
         "        tenant-create --tenant <uuid> [--key <base64>]\n"
         "        tenant-delete --tenant <uuid>\n"
         "        host-list --tenant <uuid>\n"
         "        add-record --tenant <uuid> --host <host> <FILENAME>\n"
         "        get-record --tenant <uuid> --host <host> [--time <TIMESPEC>]\n"
         "\n"
         "  TIMESPEC examples:\n"
         "        2014-01-04T13:37\n"
         "        11:13\n"
         "        -1d\n"
         "        -3h\n"
         "        now\n",
        cmdname);
}

/** Main, uses cliopt to do the dirty. */
int main (int _argc, const char *_argv[]) {
    int argc = _argc;
    const char **argv = cliopt_dispatch (CLIOPT, _argv, &argc);
    if (! argv) return 1;
    if (argc < 2) {
        usage (argv[0]);
        return 1;
    }
    const char *cmd = argv[1];
    return cliopt_runcommand (CLICMD, cmd, argc, argv);
}
