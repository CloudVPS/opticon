#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libopticondb/db_local.h>
#include <libopticon/cliopt.h>

#include "cmd.h"

optinfo OPTIONS;

#define CONCAT(a,b) a##b

#define STRINGOPT(xoptname) \
    int CONCAT(set_,xoptname) (const char *o, const char *v) { \
        OPTIONS. xoptname = v; \
        return 1; \
    }
    
#define FLAGOPT(xoptname) \
    int set_##xoptname (const char *o, const char *v) { \
        OPTIONS. xoptname = 1; \
        return 1; \
    }

STRINGOPT(tenant)
STRINGOPT(key)
STRINGOPT(path)
STRINGOPT(host)
FLAGOPT(json)
STRINGOPT(name)
STRINGOPT(meter)
STRINGOPT(description)
STRINGOPT(unit)
STRINGOPT(level)
STRINGOPT(match)
STRINGOPT(value)
STRINGOPT(weight)
STRINGOPT(api_url)
STRINGOPT(keystone_url)
STRINGOPT(keystone_token)
STRINGOPT(opticon_token)

/** Handle --type */
int set_type (const char *o, const char *v) {
    if ( (strcmp (v, "integer") == 0) ||
         (strcmp (v, "frac") == 0) ||
         (strcmp (v, "string") == 0) ) {
        OPTIONS.type = v;
        return 1;
    }
    fprintf (stderr, "%% Illegal type: %s\n", v);
    exit (1);
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
    {"--name","-n",OPT_VALUE,"",set_name},
    {"--meter","-m",OPT_VALUE,"",set_meter},
    {"--type","-Y",OPT_VALUE,"integer",set_type},
    {"--description","-D",OPT_VALUE,"",set_description},
    {"--unit","-U",OPT_VALUE,"",set_unit},
    {"--level","-L",OPT_VALUE,"alert",set_level},
    {"--match","-M",OPT_VALUE,"gt",set_match},
    {"--value","-V",OPT_VALUE,"",set_value},
    {"--weight","-W",OPT_VALUE,"1.0",set_weight},
    {"--api_url","-A",OPT_VALUE,"http://localhost:8888/",set_api_url},
    {"--keystone_url","-X",OPT_VALUE,
            "https://identity.stack.cloudvps.com/v2.0/",set_keystone_url},
    {"--keystone_token","-K",OPT_VALUE,"",set_keystone_token},
    {"--opticon-token","-O",OPT_VALUE,
            "a1bc7681-b616-4805-90c2-f9a08ce460d3",set_opticon_token},
    {NULL,NULL,0,NULL,NULL}
};

/** Sub-commands */
clicmd CLICMD[] = {
    {"tenant-list",cmd_tenant_list},
    {"tenant-create",cmd_tenant_create},
    {"tenant-delete",cmd_tenant_delete},
    {"tenant-get-metadata",cmd_tenant_get_metadata},
    {"tenant-set-metadata",cmd_tenant_set_metadata},
    {"meter-list",cmd_meter_list},
    {"meter-create",cmd_meter_create},
    {"meter-delete",cmd_meter_delete},
    {"host-list",cmd_host_list},
    {"watcher-set",cmd_watcher_set},
    {"watcher-delete",cmd_watcher_delete},
    {"watcher-list",cmd_watcher_list},
    {"host-get-record",cmd_get_record},
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
         "        tenant-create        [--tenant <uuid>] [--key <base64>] [--name <name>]\n"
         "        tenant-delete         --tenant <uuid>\n"
         "        tenant-get-metadata   --tenant <uuid>\n"
         "        tenant-set-metadata   --tenant <uuid> <key> <value>\n"
         "        meter-list            --tenant <uuid>\n"
         "        meter-create          --tenant <uuid> --meter <meterid> --type <TYPE>\n"
         "                             [--description <description>] [--unit <unitstr>]\n"
         "        meter-delete          --tenant <uuid> --meter <meterid>\n"
         "        watcher-list          --tenant <uuid> [--host <uuid>]\n"
         "        watcher-set           --tenant <uuid> [--host <uuid>]\n"
         "                              --meter <meterid>\n"
         "                              --level <warning|alert|critical>\n"
         "                              --match <gt|lt|eq> (only for tenant)\n"
         "                             [--weight <weight>] --value <value>\n"
         "        watcher-delete        --tenant <uuid> [--host <uuid>\n"
         "                              --meter <meterid>\n"
         "        host-list             --tenant <uuid>\n"
         "        host-get-record       --tenant <uuid> --host <host> [--time <TIMESPEC>]\n"
         "\n"
         "  TYPE:\n"
         "        integer, string, or frac\n"
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
