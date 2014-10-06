#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>

#include <libopticondb/db_local.h>
#include <libopticon/cliopt.h>
#include <libopticon/var_parse.h>
#include <libopticon/dump.h>
#include <libopticon/react.h>
#include <libopticon/log.h>
#include <libhttp/http.h>

#include "cmd.h"
#include "api.h"

optinfo OPTIONS;

/* Convenience macro's for setting up a shitload of command line flag
   handlers */
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
STRINGOPT(config_file)

/** Handle --type */
int set_type (const char *o, const char *v) {
    if ( (strcmp (v, "integer") == 0) ||
         (strcmp (v, "frac") == 0) ||
         (strcmp (v, "table") == 0) ||
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
    int hour,minute;
    
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

/** Write a token to the cache-file in the user's homedir */
void write_cached_token (const char *token) {
    char *home = getenv ("HOME");
    if (! home) return;
    char path[1024];
    sprintf (path, "%s/.opticon-token-cache", home);
    FILE *f = fopen (path, "w");
    if (! f) return;

    var *cache = var_alloc();
    var_set_str_forkey (cache, "keystone_token", token);
    dump_var (cache, f);
    fclose (f);
    var_free (cache);
}

/** Load a cached token from the cache-file in the user's homedir.
  * If the token is older than an hour, re-validate it against the
  * API server and refresh. */
int load_cached_token (void) {
    char *home = getenv ("HOME");
    if (! home) return 0;
    if (OPTIONS.keystone_token[0]) return 1;
    struct stat st;
    time_t tnow = time (NULL);
    
    int res = 0;
    var *cache = var_alloc();
    char path[1024];
    sprintf (path, "%s/.opticon-token-cache", home);
    if (var_load_json (cache, path)) {
        const char *token;
        token = var_get_str_forkey (cache, "keystone_token");
        
        if (token) {
            stat (path, &st);
            /* re-validate after an hour */
            if (tnow - st.st_mtime > 3600) {
                OPTIONS.keystone_token = token;
                var *vres = api_get_raw ("/token", 0);
                if (vres) {
                    OPTIONS.keystone_token = strdup (token);
                    res = 1;
                    var_free (vres);
                    
                    /* refresh the file */
                    write_cached_token (token);
                }
            }
            else {
                OPTIONS.keystone_token = strdup (token);
                res = 1;
            }
        }
    }
    var_free (cache);
    return res;
}

/** Convenience function for getting the host+domainname from a url.
  * \param url The url
  * \return Allocated string, caller should free().
  */
char *domain_from_url (const char *url) {
    char *c = strchr (url, '/');
    while (*c == '/') c++;
    char *cc = c;
    if (! (c = strchr (c, '@'))) c = cc;
    char *res = strdup (c);
    c = strchr(res, '/');
    if (c) *c = 0;
    return res;
}

/** Ask for keystone credentials and use them to attempt to
  * log into the keystone server. Write the resulting token
  * to the cache-file if succesful.
  */
int keystone_login (void) {
    char username[256];
    printf ("%% Login required\n\n");
    char *domain = domain_from_url (OPTIONS.keystone_url);
    printf ("  OpenStack Domain: %s\n", domain);
    free (domain);
    printf ("  Username........: ");
    fflush (stdout);
    fgets (username, 255, stdin);
    if (username[0]) username[strlen(username)-1] = 0;
    const char *password = getpass ("  Password........: ");

    printf ("\n");

    char *kurl = (char *) malloc (strlen (OPTIONS.keystone_url) + 10);
    sprintf (kurl, "%s/tokens", OPTIONS.keystone_url);
    var *req = var_alloc();
    var *req_auth = var_get_dict_forkey (req, "auth");
    var *req_pw = var_get_dict_forkey (req_auth, "passwordCredentials");
    var_set_str_forkey (req_pw, "username", username);
    var_set_str_forkey (req_pw, "password", password);
    var *hdr = var_alloc();
    var_set_str_forkey (hdr, "Content-Type", "application/json");
    
    var *err = var_alloc();
    var *kres = http_call ("POST", kurl, hdr, req, err, NULL);
    if (! kres) {
        printf ("%% Login failed\n");
        var_free (hdr);
        var_free (req);
        var_free (err);
        free (kurl);
        return 0;
    }
    
    var *res_xs = var_get_dict_forkey (kres, "access");
    var *res_tok = var_get_dict_forkey (res_xs, "token");
    const char *token = var_get_str_forkey (res_tok, "id");
    if (token) {
        OPTIONS.keystone_token = strdup (token);
        write_cached_token (token);
    }
    var_free (hdr);
    var_free (req);
    var_free (err);
    free (kurl);
    return (token == NULL) ? 0 : 1;
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
    {"--api-url","-A",OPT_VALUE,"",set_api_url},
    {"--keystone-url","-X",OPT_VALUE,"",set_keystone_url},
    {"--keystone-token","-K",OPT_VALUE,"",set_keystone_token},
    {"--opticon-token","-O",OPT_VALUE,"",set_opticon_token},
    {"--config-file","-c",OPT_VALUE,
            "/etc/opticon/opticon-cli.conf",set_config_file},
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
    {"host-show",cmd_get_record},
    {"session-list",cmd_session_list},
    {"dancing-bears", cmd_bears},
    {NULL,NULL}
};

/** Set up the api endpoint from configuration */
int conf_endpoint_api (const char *id, var *v, updatetype tp) {
    OPTIONS.api_url = var_get_str(v);
    return 1;
}

/** Set up the keystone endpoint from configuration */
int conf_endpoint_keystone (const char *id, var *v, updatetype tp) {
    OPTIONS.keystone_url = var_get_str(v);
    return 1;
}

/** Set up the default --tenant argument from configuration.
  * No-op if an explicit --tenant flag was already provided.
  */
int conf_default_tenant (const char *id, var *v, updatetype tp) {
    if (OPTIONS.tenant[0] == 0) {
        OPTIONS.tenant = var_get_str (v);
    }
    return 1;
}

int conf_admin_token (const char *id, var *v, updatetype tp) {
    if (OPTIONS.opticon_token[0] == 0) {
        OPTIONS.opticon_token = var_get_str (v);
    }
    return 1;
}

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
         "        host-show             --tenant <uuid> --host <host> [--time <TIMESPEC>]\n"
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
    struct stat st;
    const char **argv = cliopt_dispatch (CLIOPT, _argv, &argc);
    if (! argv) return 1;
    if (argc < 2) {
        usage (argv[0]);
        return 1;
    }

    opticonf_add_reaction ("endpoints/keystone", conf_endpoint_keystone);
    opticonf_add_reaction ("endpoints/opticon", conf_endpoint_api);
    opticonf_add_reaction ("defaults/tenant", conf_default_tenant);
    opticonf_add_reaction ("defaults/admin_token", conf_admin_token);

    OPTIONS.conf = var_alloc();
    
    if (stat (OPTIONS.config_file, &st) == 0) {
        if (! var_load_json (OPTIONS.conf, OPTIONS.config_file)) {
            log_error ("Error loading %s: %s\n",
                       OPTIONS.config_file, parse_error());
            return 1;
        }
    }
    
    char *rcpath = malloc(strlen(getenv("HOME")) + 32);
    sprintf (rcpath, "%s/.opticonrc", getenv("HOME"));
    
    if (stat (rcpath, &st) == 0) {
        if (! var_load_json (OPTIONS.conf, rcpath)) {
            log_error ("Error loading %s: %s\n", rcpath, parse_error());
            return 1;
        }
    }
    
    free (rcpath);
    opticonf_handle_config (OPTIONS.conf);
    if (OPTIONS.api_url[0] == 0) {
        fprintf (stderr, "%% No opticon endpoint found\n");
        return 1;
    }
    if (OPTIONS.keystone_url[0] == 0 && OPTIONS.opticon_token[0] == 0) {
        fprintf (stderr, "%% No keystone endpoint found\n");
        return 1;
    }

    if (OPTIONS.keystone_token[0] == 0 && OPTIONS.opticon_token[0] == 0) {
        if (! load_cached_token()) {
            if (! keystone_login()) return 1;
        }
    }
    const char *cmd = argv[1];
    return cliopt_runcommand (CLICMD, cmd, argc, argv);
}
