#include "cmd.h"
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <libopticon/datatypes.h>
#include <libopticon/aes.h>
#include <libopticon/codec_json.h>
#include <libopticon/ioport_file.h>
#include <libopticon/util.h>
#include <libopticon/var.h>
#include <libopticon/dump.h>
#include <libopticon/defaultmeters.h>
#include <libopticondb/db_local.h>
#include <libhttp/http.h>

#include "import.h"

var *api_call (const char *mth, var *data, const char *fmt, ...)
{
    char path[1024];
    path[0] = 0;
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (path, 1023, fmt, ap);
    va_end (ap);
    
    char tmpurl[1024];
    var *outhdr = var_alloc();
    if (OPTIONS.keystone_token[0]) {
        var_set_str_forkey (outhdr, "X-Auth-Token", OPTIONS.keystone_token);
    }
    else if (OPTIONS.opticon_token[0]) {
        var_set_str_forkey (outhdr, "X-Opticon-Token", OPTIONS.opticon_token);
    }
    strcpy (tmpurl, OPTIONS.api_url);
    int len = strlen (tmpurl);
    if (! len) { var_free (outhdr); return NULL; }
    
    if (tmpurl[len-1] == '/') tmpurl[len-1] = 0;
    strncat (tmpurl, path, 1023);
    tmpurl[1023] = 0;
    var *res = http_call (mth, tmpurl, outhdr, data, NULL);
    var_free (outhdr);
    return res;
}

var *api_get (const char *fmt, ...) {
    char path[1024];
    path[0] = 0;
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (path, 1023, fmt, ap);
    va_end (ap);
    
    char tmpurl[1024];
    var *outhdr = var_alloc();
    var *data = var_alloc();
    if (OPTIONS.keystone_token[0]) {
        var_set_str_forkey (outhdr, "X-Auth-Token", OPTIONS.keystone_token);
    }
    else if (OPTIONS.opticon_token[0]) {
        var_set_str_forkey (outhdr, "X-Opticon-Token", OPTIONS.opticon_token);
    }
    strcpy (tmpurl, OPTIONS.api_url);
    int len = strlen (tmpurl);
    if (! len) {
        var_free (outhdr);
        var_free (data);
        return NULL;
    }
    
    if (tmpurl[len-1] == '/') tmpurl[len-1] = 0;
    strncat (tmpurl, path, 1023);
    tmpurl[1023] = 0;
    var *res = http_call ("GET", tmpurl, outhdr, data, NULL);
    var_free (outhdr);
    var_free (data);
    return res;
}

/** The tenant-list command */
int cmd_tenant_list (int argc, const char *argv[]) {
    printf ("UUID                                 Hosts  Name\n");
    printf ("---------------------------------------------"
            "----------------------------------\n");

    var *res = api_get ("/");
    if (res) {
        var *res_tenant = var_get_array_forkey (res, "tenant");
        if (var_get_count (res_tenant)) {
            var *crsr = res_tenant->value.arr.first;
            while (crsr) {
                printf ("%s %5llu  %s\n",
                        var_get_str_forkey (crsr, "id"),
                        var_get_int_forkey (crsr, "hostcount"),
                        var_get_str_forkey (crsr, "name"));
                crsr = crsr->next;
            }
        }
    }
    var_free (res);
    return 0;
}

/** The tenant-get-metadata command */
int cmd_tenant_get_metadata (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }

    var *meta = api_get ("/%s/meta", OPTIONS.tenant);
    dump_var (meta, stdout);
    var_free (meta);
    return 0;
}

/** The tenant-set-metadata command */
int cmd_tenant_set_metadata (int argc, const char *argv[]) {
    if (argc < 4) {
        fprintf (stderr, "%% Missing key and value arguments\n");
        return 1;
    }
    
    const char *key = argv[2];
    const char *val = argv[3];
    
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }

    var *api_meta = api_get ("/%s/meta", OPTIONS.tenant);
    var *meta = var_get_dict_forkey (api_meta, "metadata");
    var_set_str_forkey (meta, key, val);
    
    var *res = api_call ("POST", api_meta, "/%s/meta", OPTIONS.tenant);
    var_free (api_meta);
    var_free (res);
    return 0;
}

int cmd_tenant_add_meter (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    if (OPTIONS.meter[0] == 0) {
        fprintf (stderr, "%% No meter provided\n");
        return 1;
    }

    var *req = var_alloc();
    var *req_meter = var_get_dict_forkey (req, "meter");
    var_set_str_forkey (req_meter, "type", OPTIONS.type);
    if (OPTIONS.description[0]) {
        var_set_str_forkey (req_meter, "description", OPTIONS.description);
    }
    if (OPTIONS.unit[0]) {
        var_set_str_forkey (req_meter, "unit", OPTIONS.unit);
    }
    
    var *apires = api_call ("POST",req,"/%s/meter/%s",
                            OPTIONS.tenant, OPTIONS.meter);

    var_free (req);
    var_free (apires);
    return 0;
}

int cmd_tenant_delete_meter (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    if (OPTIONS.meter[0] == 0) {
        fprintf (stderr, "%% No meter provided\n");
        return 1;
    }
    
    var *p = var_alloc();
    var *apires = api_call ("DELETE", p, "/%s/meter/%s",
                            OPTIONS.tenant, OPTIONS.meter);
    var_free (p);
    var_free (apires);
    return 0;
}    

int cmd_tenant_set_watcher (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    if (OPTIONS.meter[0] == 0) {
        fprintf (stderr, "%% No meter provided\n");
        return 1;
    }
    if (OPTIONS.match[0] == 0) {
        fprintf (stderr, "%% No match provided\n");
        return 1;
    }
    if (OPTIONS.value[0] == 0) {
        fprintf (stderr, "%% No value provided\n");
        return 1;
    }
    
    var *mdef = api_get ("/%s/meter", OPTIONS.tenant);
    if (! mdef) return 1;
    
    var *mdef_m = var_get_dict_forkey (mdef, "meter");
    var *mde_meter = var_get_dict_forkey (mdef_m, OPTIONS.meter);
    const char *inftype = var_get_str_forkey (mde_meter, "type");
    
    var *req = var_alloc();
    var *req_watcher = var_get_dict_forkey (req, "watcher");
    var *reql = var_get_dict_forkey (req_watcher, OPTIONS.level);
    
    var_set_str_forkey (reql, "cmp", OPTIONS.match);
    
    if (strcmp (inftype, "integer") == 0) {
        var_set_int_forkey (reql, "val", strtoull (OPTIONS.value, NULL, 10));
    }
    else if (strcmp (inftype, "frac") == 0) {
        var_set_double_forkey (reql, "val", atof (OPTIONS.value));
    }
    else {
        var_set_str_forkey (reql, "val", OPTIONS.value);
    }
    
    var_set_double_forkey (reql, "weight", atof (OPTIONS.weight));
    
    dump_var (req, stdout);
    
    var *apires = api_call ("POST", req, "/%s/watcher/%s",
                            OPTIONS.tenant, OPTIONS.meter);
    var_free (mdef);
    var_free (req);
    var_free (apires);
    return 0;
}

int cmd_tenant_delete_watcher (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    if (OPTIONS.meter[0] == 0) {
        fprintf (stderr, "%% No meter provided\n");
        return 1;
    }
    
    var *req = var_alloc();
    var *apires = api_call ("DELETE", req, "/%s/watcher/%s",
                            OPTIONS.tenant, OPTIONS.meter);
    var_free (req);
    var_free (apires);
    return 0;    
}

int cmd_host_set_watcher (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    if (OPTIONS.host[0] == 0) {
        fprintf (stderr, "%% No host provided\n");
        return 1;
    }
    if (OPTIONS.meter[0] == 0) {
        fprintf (stderr, "%% No meter provided\n");
        return 1;
    }
    if (OPTIONS.value[0] == 0) {
        fprintf (stderr, "%% No value provided\n");
        return 1;
    }
    
    var *mdef = api_get ("/%s/meter", OPTIONS.tenant);
    if (! mdef) return 1;
    
    var *mdef_m = var_get_dict_forkey (mdef, "meter");
    var *mde_meter = var_get_dict_forkey (mdef_m, OPTIONS.meter);
    const char *inftype = var_get_str_forkey (mde_meter, "type");
    

    var *req = var_alloc();
    var *v_watcher = var_get_dict_forkey (req, "watcher");
    var *v_thislevel = var_get_dict_forkey (v_watcher, OPTIONS.level);
    var_set_double_forkey (v_thislevel, "weight", atof(OPTIONS.weight));
    if (strcmp (inftype, "int") == 0) {
        var_set_int_forkey (v_thislevel, "val",
                            strtoull (OPTIONS.value, NULL, 10));
    }
    else if (strcmp (inftype, "frac") == 0) {
        var_set_double_forkey (v_thislevel, "val", atof (OPTIONS.value));
    }
    else var_set_str_forkey (v_thislevel, "val", OPTIONS.value);
    
    dump_var (req, stdout);
    
    var *apires = api_call ("POST",req, "/%s/host/%s/watcher/%s",
                            OPTIONS.tenant, OPTIONS.host,
                            OPTIONS.meter);
    
    var_free (mdef);
    var_free (req);
    var_free (apires);
    return 0;
}

int cmd_host_delete_watcher (int argc, const char *argv[]) {
    uuid tenant;
    uuid host;
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    tenant = mkuuid (OPTIONS.tenant);
    if (OPTIONS.host[0] == 0) {
        fprintf (stderr, "%% No host provided\n");
        return 1;
    }
    host = mkuuid (OPTIONS.host);
    if (OPTIONS.meter[0] == 0) {
        fprintf (stderr, "%% No meter provided\n");
        return 1;
    }

    db *DB = localdb_create (OPTIONS.path);
    if (! db_open (DB, tenant, NULL)) {
        fprintf (stderr, "%% Could not open %s\n", OPTIONS.tenant);
        db_free (DB);
        return 1;
    }

    var *meta = db_get_hostmeta (DB, host);
    if (! meta) meta = var_alloc();
    var *v_meter = var_get_dict_forkey (meta, "meter");
    var *v_thismeter = var_get_dict_forkey (v_meter, OPTIONS.meter);
    var_delete_key (v_thismeter, OPTIONS.level);
    db_set_hostmeta (DB, host, meta);
    db_free (DB);
    return 0;
}

var *collect_meterdefs (uuid tenant, uuid host) {
    db *DB = localdb_create (OPTIONS.path);
    if (! db_open (DB, tenant, NULL)) {
        fprintf (stderr, "%% Could not open %s\n", OPTIONS.tenant);
        db_free (DB);
        return NULL;
    }
    
    const char *triggers[3] = {"warning","alert","critical"};
    
    var *tenantmeta = db_get_metadata (DB);
    if (!tenantmeta) tenantmeta = var_alloc();
    var *hostmeta = db_get_hostmeta (DB, host);
    if (!hostmeta) hostmeta = var_alloc();
    
    var *conf = var_alloc();
    var *cmeters = var_get_dict_forkey (conf, "meter");
    var_copy (cmeters, get_default_meterdef());
    var *tmeters = var_get_dict_forkey (tenantmeta, "meter");
    var *hmeters = var_get_dict_forkey (hostmeta, "meter");
    const char *tstr;
    uint64_t tint;
    double tfrac;
    var *tvar;
    
    var *tc = tmeters->value.arr.first;
    while (tc) {
        var *cc = var_get_dict_forkey (cmeters, tc->id);
        tstr = var_get_str_forkey (tc, "type");
        if (tstr) var_set_str_forkey (cc, "type", tstr);
        tstr = var_get_str_forkey (tc, "description");
        if (tstr) var_set_str_forkey (cc, "description", tstr);
        tstr = var_get_str_forkey (tc, "unit");
        if (tstr) var_set_str_forkey (cc, "unit", tstr);
        
        for (int tr=0; tr<3; ++tr) {
            tvar = var_find_key (tc, triggers[tr]);
            if (tvar) {
                var *ctrig = var_get_dict_forkey (cc, triggers[tr]);
                var_copy (ctrig, tvar);
                var_set_str_forkey (ctrig, "origin", "tenant");
            }
        }
        tc = tc->next;
    }
    var *hc = NULL;
    if (host.msb || host.lsb) {
        hc = hmeters->value.arr.first;
    }
    while (hc) {
        var *cc = var_get_dict_forkey (cmeters, hc->id);
        tstr = var_get_str_forkey (hc, "type");
        if (! tstr) {
            fprintf (stderr, "%% No type set for meter %s in "
                             "host watchers\n", hc->id);
            return NULL;
        }
        const char *origtype = var_get_str_forkey (cc, "type");
        if (strcmp (tstr, origtype) != 0) {
            fprintf (stderr, "%% Type %s set for watcher %s in host "
                             "doesn't match definition %s\n", 
                             tstr, hc->id, origtype);
            return NULL;
        }
        
        for (int tr=0; tr<3; ++tr) {
            tvar = var_find_key (hc, triggers[tr]);
            if (tvar) {
                var *cdef = var_find_key (cc, triggers[tr]);
                var *val = var_find_key (tvar, "val");
                var *cval = var_find_key (cdef, "val");
                if (! cval) {
                    fprintf (stderr, "%% Ill-defined value in cdef "
                                     "of %s at %s\n", hc->id, triggers[tr]);
                    return NULL;
                }
                if (val) {
                    var_copy (cval, val);
                }
                var_set_str_forkey (cdef, "origin", "host");
                var_set_double_forkey (cdef, "weight",
                            var_get_double_forkey (tvar, "weight"));
            }
        }
        
        hc = hc->next;
    }
    var_free (tenantmeta);
    var_free (hostmeta);
    return conf;
}

void print_data (const char *meterid, const char *trig, var *v) {
    const char *origin = var_get_str_forkey (v, "origin");
    if (! origin) origin = "default";
    char valst[64];
    var *val = var_find_key (v, "val");
    if (! val) return;
    
    switch (val->type) {
        case VAR_INT:
            sprintf (valst, "%llu", var_get_int (val));
            break;
        
        case VAR_DOUBLE:
            sprintf (valst, "%.2f", var_get_double (val));
            break;
        
        case VAR_STR:
            strncpy (valst, var_get_str (val), 63);
            valst[63] = 0;
            break;
        
        default:
            strcpy (valst, "<error %i>");
            break;
    }
    
    printf ("%-8s %-12s %-9s %-6s %21s %18.1f\n", origin, meterid,
            trig, var_get_str_forkey (v, "cmp"), valst,
            var_get_double_forkey (v, "weight"));
}

int cmd_host_list_watchers (int argc, const char *argv[]) {
    uuid tenant;
    uuid host;
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    
    printf ("From     Meter        Trigger   Match                  "
            "Value             Weight\n"
            "-------------------------------------------------------"
            "------------------------\n");

    var *apires;
    
    if (OPTIONS.host[0]) {
        apires = api_get ("/%s/host/%s/watcher", OPTIONS.tenant, OPTIONS.host);
    }
    else {
        apires = api_get ("/%s/watcher", OPTIONS.tenant);
    }
    var *apiwatch = var_get_dict_forkey (apires, "watcher");
    if (var_get_count (apiwatch)) {
        var *crsr = apiwatch->value.arr.first;
        while (crsr) {
            print_data (crsr->id, "warning",
                        var_get_dict_forkey (crsr, "warning"));
            print_data (crsr->id, "alert",
                        var_get_dict_forkey (crsr, "alert"));
            print_data (crsr->id, "critical",
                        var_get_dict_forkey (crsr, "critical"));
            crsr = crsr->next;
        }
    }
    var_free (apires);
    return 0;
}

/** The tenant-delete command */
int cmd_tenant_delete (int argc, const char *argv[]) {
   uuid tenant;
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    
    var *req = var_alloc();
    var *apires = api_call ("DELETE", req, "/%s", OPTIONS.tenant);
    if (apires) dump_var (apires, stdout);
    var_free (req);
    var_free (apires);
    return 0;
}

/** The tenant-create command */
int cmd_tenant_create (int argc, const char *argv[]) {
    uuid tenant;
    aeskey key;
    char *strkey;
    
    if (OPTIONS.tenant[0] == 0) {
        tenant = uuidgen();
        OPTIONS.tenant = (const char *) malloc (40);
        uuid2str (tenant, (char *) OPTIONS.tenant);
    }
    else {
        tenant = mkuuid (OPTIONS.tenant);
    }
    
    var *req = var_alloc();
    var *req_tenant = var_get_dict_forkey (req, "tenant");
    if (OPTIONS.key[0]) {
        var_set_str_forkey (req_tenant, "key", OPTIONS.key);
    }
    if (OPTIONS.name[0]) {
        var_set_str_forkey (req_tenant, "name", OPTIONS.name);
    }
    
    var *apires = api_call ("POST", req, "/%s", OPTIONS.tenant);
    if (apires) {
        var *r = var_get_dict_forkey (apires, "tenant");
        printf ("Tenant created:\n"
                "---------------------------------------------"
                "----------------------------------\n"
                "     Name: %s\n"
                "     UUID: %s\n"
                "  AES Key: %s\n"
                "---------------------------------------------"
                "----------------------------------\n",
                var_get_str_forkey (r, "name"),
                OPTIONS.tenant,
                var_get_str_forkey (r, "key"));
    }
    
    var_free (req);
    var_free (apires);
    return 0;
}

/** Format a timestamp for output */
static char *timfmt (time_t w, int json) {
    struct tm tm;
    if (json) gmtime_r (&w, &tm);
    else localtime_r (&w, &tm);
    char *res = (char *) malloc (24);
    strftime (res, 23, json ? "%FT%H:%M:%S" : "%F %H:%M", &tm);
    return res;
}

/** The host-list command */
int cmd_host_list (int argc, const char *argv[]) {
    uuid tenant;
    char *str_early;
    char *str_late;
    const char *unit = "KB";
    char uuidstr[40];
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }

    tenant = mkuuid (OPTIONS.tenant);
    db *DB = localdb_create (OPTIONS.path);
    if (! db_open (DB, tenant, NULL)) {
        fprintf (stderr, "%% Could not open %s\n", OPTIONS.tenant);
        db_free (DB);
        return 1;
    }

    if (OPTIONS.json) {
        printf ("\"hosts\":{\n");
    }
    else {
        printf ("UUID                                    Size "
                "First record      Last record\n");
        printf ("---------------------------------------------"
                "----------------------------------\n");
    }
    
    usage_info usage;
    int count;
    uuid *list = db_list_hosts (DB, &count);
    for (int i=0; i<count; ++i) {
        uuid2str (list[i], uuidstr);
        db_get_usage (DB, &usage, list[i]);
        str_early = timfmt (usage.earliest, OPTIONS.json);
        str_late = timfmt (usage.last, OPTIONS.json);
        if (OPTIONS.json) {
            printf ("    \"%s\":{\n", uuidstr);
            printf ("        \"usage\":%llu,\n", usage.bytes);
            printf ("        \"start\":\"%s\",\n", str_early);
            printf ("        \"end\":\"%s\"\n", str_late);
            if ((i+1) < count) {
                printf ("    },\n");
            }
            else {
                printf ("    }\n");
            }
        }
        else {
            unit = "KB";
            usage.bytes = usage.bytes / 1024;
            if (usage.bytes > 2048) {
                unit = "MB";
                usage.bytes = usage.bytes / 1024;
            }
            printf ("%s %4llu %s %s  %s\n", uuidstr, usage.bytes, unit,
                    str_early, str_late);
        }
        free (str_early);
        free (str_late);
    }
    if (OPTIONS.json) printf ("}\n");
    db_free (DB);
    free (list);
    return 0;
}

/** The add-record command */
int cmd_add_record (int argc, const char *argv[]) {
    uuid tenantid, hostid;
    
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    
    tenantid = mkuuid (OPTIONS.tenant);

    if (OPTIONS.host[0] == 0) {
        fprintf (stderr, "%% No hostid provided\n");
        return 1;
    }
    
    hostid = mkuuid (OPTIONS.host);
    
    if (argc < 3) {
        fprintf (stderr, "%% No filename provided\n");
        return 1;
    }
    
    host *H = host_alloc();
    H->uuid = hostid;
    char *json = load_file (argv[2]);
    if (! json) {
        fprintf (stderr, "%% Could not load %s\n", argv[2]);
        return 1;
    }
    
    if (! import_json (H, json)) {
        free (json);
        host_delete (H);
        return 1;
    }
    
    db *DB = localdb_create (OPTIONS.path);
    if (! db_open (DB, tenantid, NULL)) {
        fprintf (stderr, "%% Could not open database for "
                 "tenant %s\n", OPTIONS.tenant);
        free (json);
        host_delete (H);
        db_free (DB);
        return 1;
    }
    
    if (! db_save_record (DB, time(NULL), H)) {
        fprintf (stderr, "%% Error saving record\n");
        free (json);
        host_delete (H);
        db_free (DB);
        return 1;
    }

    free (json);    
    host_delete (H);
    db_free (DB);
    return 0;
}

/** The get-recrod command */
int cmd_get_record (int argc, const char *argv[]) {
    uuid tenantid, hostid;
    
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    
    tenantid = mkuuid (OPTIONS.tenant);

    if (OPTIONS.host[0] == 0) {
        fprintf (stderr, "%% No hostid provided\n");
        return 1;
    }
    
    hostid = mkuuid (OPTIONS.host);

    host *H = host_alloc();
    H->uuid = hostid;
    db *DB = localdb_create (OPTIONS.path);
    if (! db_open (DB, tenantid, NULL)) {    
        fprintf (stderr, "%% Could not open database for "
                 "tenant %s\n", OPTIONS.tenant);
        host_delete (H);
        db_free (DB);
        return 1;
    }
    
    if (! db_get_record (DB, OPTIONS.time, H)) {
        fprintf (stderr, "%% Error loading record\n");
        host_delete (H);
        db_free (DB);
        return 1;
    }
    
    ioport *out = ioport_create_filewriter (stdout);
    codec *c = codec_create_json();
    
    codec_encode_host (c, out, H);
    codec_release (c);
    ioport_close (out);
    db_close (DB);
    db_free (DB);
    host_delete (H);
    return 0;
}
