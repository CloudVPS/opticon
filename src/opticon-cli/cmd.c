#include "cmd.h"
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

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

#include "api.h"

/** The tenant-list command */
int cmd_tenant_list (int argc, const char *argv[]) {
    printf ("UUID                                 Hosts  Name\n");
    printf ("---------------------------------------------"
            "-----------------------------------\n");

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
        printf ("----------------------------------------------"
                "----------------------------------\n");
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

/** The meter-create command */
int cmd_meter_create (int argc, const char *argv[]) {
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

/** The meter-delete command */
int cmd_meter_delete (int argc, const char *argv[]) {
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

/** The meter-list command */
int cmd_meter_list (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    
    var *apires = api_get ("/%s/meter", OPTIONS.tenant);
    var *res_meter = var_get_dict_forkey (apires, "meter");
    if (var_get_count (res_meter)) {
        printf ("From     Meter        Type      Unit    Description\n");
        printf ("----------------------------------------"
                "----------------------------------------\n");
        var *crsr = res_meter->value.arr.first;
        while (crsr) {
            const char *desc = var_get_str_forkey (crsr, "description");
            const char *type = var_get_str_forkey (crsr, "type");
            const char *unit = var_get_str_forkey (crsr, "unit");
            const char *org = var_get_str_forkey (crsr, "origin");
            
            if (!desc) desc = "-";
            if (!unit) unit = "";
            if (!org) org = "default";
            
            printf ("%-8s %-12s %-8s  %-7s %s\n", org, crsr->id,
                    type, unit, desc);
            crsr = crsr->next;
        }
        printf ("---------------------------------------------"
                "-----------------------------------\n");
    }
    var_free (apires);
    return 0;
}

/** The watcher-set command */
int cmd_watcher_set (int argc, const char *argv[]) {
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
    
    if (OPTIONS.host[0] == 0) {
        var_set_str_forkey (reql, "cmp", OPTIONS.match);
    }
    
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
    
    var *apires;
    if (OPTIONS.host[0]) {
        apires = api_call ("POST", req, "/%s/host/%s/watcher/%s",
                           OPTIONS.tenant, OPTIONS.host, OPTIONS.meter);
    }
    else {
        apires = api_call ("POST", req, "/%s/watcher/%s",
                            OPTIONS.tenant, OPTIONS.meter);
    }
    var_free (mdef);
    var_free (req);
    var_free (apires);
    return 0;
}

/** The watcher-delete command */
int cmd_watcher_delete (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    if (OPTIONS.meter[0] == 0) {
        fprintf (stderr, "%% No meter provided\n");
        return 1;
    }
    
    var *req = var_alloc();
    var *apires;
    
    if (OPTIONS.host[0]) {
        apires = api_call ("DELETE", req, "/%s/host/%s/watcher/%s",
                           OPTIONS.tenant, OPTIONS.host, OPTIONS.meter);
    }
    else {
        apires = api_call ("DELETE", req, "/%s/watcher/%s",
                            OPTIONS.tenant, OPTIONS.meter);
    }
    var_free (req);
    var_free (apires);
    return 0;    
}

/** Screen display function for watcher-related data */
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

/** The watcher-list command */
int cmd_watcher_list (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    
    printf ("From     Meter        Trigger   Match                  "
            "Value             Weight\n"
            "-------------------------------------------------------"
            "-------------------------\n");

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
        printf ("---------------------------------------------"
                "-----------------------------------\n");
    }
    var_free (apires);
    return 0;
}

/** If OPTIONS.tenant is the default, unset it */
void disregard_default_tenant (void) {
    var *conf_defaults = var_get_dict_forkey (OPTIONS.conf, "defaults");
    const char *deftenant = var_get_str_forkey (conf_defaults, "tenant");
    if (! deftenant) return;
    if (strcmp (deftenant, OPTIONS.tenant) == 0) OPTIONS.tenant = "";
}

/** The tenant-delete command */
int cmd_tenant_delete (int argc, const char *argv[]) {
    /* Avoid using the default tenant in this case */
    disregard_default_tenant();

    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    
    var *req = var_alloc();
    var *apires = api_call ("DELETE", req, "/%s", OPTIONS.tenant);
    var_free (req);
    var_free (apires);
    return 0;
}

/** The tenant-create command */
int cmd_tenant_create (int argc, const char *argv[]) {
    uuid tenant;
    
    /* Avoid using the default tenant in this case */
    disregard_default_tenant();
    
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
                "-----------------------------------\n"
                "     Name: %s\n"
                "     UUID: %s\n"
                "  AES Key: %s\n"
                "---------------------------------------------"
                "-----------------------------------\n",
                var_get_str_forkey (r, "name"),
                OPTIONS.tenant,
                var_get_str_forkey (r, "key"));
    }
    
    var_free (req);
    var_free (apires);
    return 0;
}

/** The host-list command */
int cmd_host_list (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }

    const char *unit;
    
    var *apires = api_get ("/%s/host", OPTIONS.tenant);
    var *v_hosts = var_get_array_forkey (apires, "host");
    if (var_get_count (v_hosts)) {
        printf ("UUID                                    Size "
                "First record      Last record\n");
        printf ("---------------------------------------------"
                "-----------------------------------\n");
        var *crsr = v_hosts->value.arr.first;
        
        while (crsr) {
            uint64_t usage = var_get_int_forkey (crsr, "usage");
            unit = "KB";
            usage = usage / 1024;
            if (usage > 2048) {
                unit = "MB";
                usage = usage / 1024;
            }
            
            char start[24];
            char end[24];
            
            strncpy (start, var_get_str_forkey (crsr, "start"), 23);
            strncpy (end, var_get_str_forkey (crsr, "end"), 23);
            start[16] = 0;
            end[16] = 0;
            start[10] = ' ';
            end[10] = ' ';

            printf ("%s %4llu %s %s  %s\n",
                    var_get_str_forkey (crsr, "id"),
                    usage, unit, start, end);
            crsr = crsr->next;
        }
        printf ("---------------------------------------------"
                "-----------------------------------\n");
    }

    var_free (apires);
    return 0;
}

/** Display function for host-show section headers */
void print_hdr (const char *hdr) {
    const char *mins = "-----------------------------------------------"
                      "-----------------------------------------------"
                      "-----------------------------------------------";
    
    int minspos = strlen(mins) - 73;
    const char *crsr = hdr;
    printf ("---( ");
    while (*crsr) {
        putc (toupper (*crsr), stdout);
        minspos++;
        crsr++;
    }
    printf (" )");
    printf ("%s", mins + minspos);
    putc ('\n', stdout);
}

/** Display function for host-show data */
void print_value (const char *key, const char *fmt, ...) {
    char val[4096];
    val[0] = 0;
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (val, 4096, fmt, ap);
    va_end (ap);

    const char *dots = "....................";
    int dotspos = strlen(dots) - 16;
    printf ("%s", key);
    dotspos += strlen (key);
    if (dotspos < strlen (dots)) printf ("%s", dots+dotspos);
    printf (": ");
    printf ("%s", val);
    printf ("\n");
}

/** Print out an array as a comma-separated list */
void print_array (const char *key, var *arr) {
    char out[4096];
    out[0] = 0;
    int cnt=0;
    var *crsr = arr->value.arr.first;
    while (crsr) {
        if (cnt) strncat (out, ",", 4096);
        switch (crsr->type) {
            case VAR_INT:
                snprintf (out+strlen(out), 4095-strlen(out), "%llu",
                          var_get_int (crsr));
                break;
            
            case VAR_DOUBLE:
                snprintf (out+strlen(out), 4095-strlen(out), "%.2f",
                          var_get_double (crsr));
                break;
            
            case VAR_STR:
                strncat (out, var_get_str (crsr), 4096);
                break;
            
            default:
                strncat (out, "?", 4096);
                break;
        }
        crsr = crsr->next;
        cnt++;
    }
    
    print_value (key, "%s", out);   
}

/** Table alignment indicator */
typedef enum {
    CA_NULL,
    CA_L,
    CA_R
} columnalign;

void print_values (var *apires, const char *pfx) {
    var *mdef = api_get ("/%s/meter", OPTIONS.tenant);
    var *meters = var_get_dict_forkey (mdef, "meter");
    var *crsr = apires->value.arr.first;
    while (crsr) {
        char valbuf[1024];
        const char *name = NULL;
        var *meter;
        if (pfx) {
            sprintf (valbuf, "%s/%s", pfx, crsr->id);
            meter = var_get_dict_forkey (meters, valbuf);
        }
        else meter = var_get_dict_forkey (meters, crsr->id);
        name = var_get_str_forkey (meter, "description");
        if (! name) name = crsr->id;
        switch (crsr->type) {
            case VAR_ARRAY:
                print_array (name, crsr);
                break;
            
            case VAR_STR:
                print_value (name, var_get_str(crsr));
                break;
            
            case VAR_INT:
                sprintf (valbuf, "%llu", var_get_int(crsr));
                print_value (name, valbuf);
                break;
            
            case VAR_DOUBLE:
                sprintf (valbuf, "%.2f", var_get_double(crsr));
                print_value (name, valbuf);
                break;
            
            case VAR_DICT:
                print_values (crsr, crsr->id);
                break;
                
            default:
                break;
        }
        crsr = crsr->next;
    }
    var_free (mdef);
}

/** Print out tabular data (like top, df) with headers, bells and
  * whistles.
  * \param arr The array-of-dicts to print
  * \param hdr Array of strings containing column headers. NULL terminated.
  * \param fld Array of strings representing column field ids.
  * \param align Array of columnalign for left/right alignment of columns.
  * \param typ Array of data types.
  * \param wid Array of integers representing column widths.
  * \param suffx Array of suffixes to print after specific column cells.
  * \param div Number to divide a VAR_INT value by prior to printing. The
  *            resulting number will be represented as a float with two
  *            decimals.
  */
void print_table (var *arr, const char **hdr, const char **fld,
                  columnalign *align, vartype *typ, int *wid,
                  const char **suffx, int *div) {
    char fmt[16];
    char buf[1024];
    
    int col = 0;
    while (hdr[col]) {
        strcpy (fmt, "%");
        if (align[col] == CA_L) strcat (fmt, "-");
        if (wid[col]) sprintf (fmt+strlen(fmt), "%i", wid[col]);
        strcat (fmt, "s ");
        printf (fmt, hdr[col]);
        col++;
    }
    printf ("\n");
    
    var *node = arr->value.arr.first;
    while (node) {
        col = 0;
        while (hdr[col]) {
            switch (typ[col]) {
                case VAR_STR:
                    strncpy (buf, var_get_str_forkey (node, fld[col]),512);
                    buf[512] = 0;
                    break;
                
                case VAR_INT:
                    if (div[col]) {
                        double nval =
                            (double) var_get_int_forkey (node, fld[col]);
                        
                        nval = nval / ((double) div[col]);
                        sprintf (buf, "%.2f", nval);
                    }
                    else {
                        sprintf (buf, "%llu",
                                 var_get_int_forkey (node, fld[col]));
                    }
                    break;
                
                case VAR_DOUBLE:
                    sprintf (buf, "%.2f",
                             var_get_double_forkey (node, fld[col]));
                    break;
                
                default:
                    buf[0] = 0;
                    break;
            }
            strcat (buf, suffx[col]);
            strcpy (fmt, "%");
            if (align[col] == CA_L) strcat (fmt, "-");
            if (wid[col]) sprintf (fmt+strlen(fmt), "%i", wid[col]);
            strcat (fmt, "s ");
            printf (fmt, buf);
            col++;
        }
        printf ("\n");
        node = node->next;
    }
}

/** The get-recrod command */
int cmd_get_record (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    if (OPTIONS.host[0] == 0) {
        fprintf (stderr, "%% No hostid provided\n");
        return 1;
    }

    var *apires = api_get ("/%s/host/%s", OPTIONS.tenant, OPTIONS.host);
    
    if (OPTIONS.json) {
        dump_var (apires, stdout);
        var_free (apires);
        return 0;
    }
    
    #define Arr(x) var_get_array_forkey(apires,x)
    #define Vint(x) var_get_int_forkey(apires,x)
    #define Vstr(x) var_get_str_forkey(apires,x)
    #define Vfrac(x) var_get_double_forkey(apires,x)
    #define VDint(x,y) var_get_int_forkey(var_get_dict_forkey(apires,x),y)
    #define VDstr(x,y) var_get_str_forkey(var_get_dict_forkey(apires,x),y)
    #define VDfrac(x,y) var_get-double_forkey(var_get_dict_forkey(apires,x),y)
    #define VAfrac(x,y) var_get_double_atindex(var_get_array_forkey(apires,x),y)
    #define Vdone(x) var_delete_key(apires,x)
    /* -------------------------------------------------------------*/
    print_hdr ("HOST");
    print_value ("UUID", "%s", OPTIONS.host);
    print_value ("Hostname", "%s", Vstr("hostname"));
    print_value ("Address", "%s", VDstr("agent","ip"));
    print_value ("Status", "%s", Vstr("status"));
    
    print_array ("Problems", Arr("problems"));
    
    Vdone("hostname");
    Vdone("agent");
    Vdone("status");
    Vdone("problems");
    
    char uptimestr[128];
    uint64_t uptime = Vint("uptime"); Vdone("uptime");
    uint64_t u_days = uptime / 86400ULL;
    uint64_t u_hours = (uptime - (86400 * u_days)) / 3600ULL;
    uint64_t u_mins = (uptime - (86400 * u_days) - (3600 * u_hours)) / 60ULL;
    uint64_t u_sec = uptime % 60;
    
    if (u_days) {
        sprintf (uptimestr, "%llu day%s, %llu:%02llu:%02llu", u_days,
                 (u_days==1)?"":"s", u_hours, u_mins, u_sec);
    }
    else if (u_hours) {
        sprintf (uptimestr, "%llu:%02llu:%02llu", u_hours, u_mins, u_sec);
    }
    else {
        sprintf (uptimestr, "%llu minute%s, %llu second%s",
                 u_mins, (u_mins==1)?"":"s", u_sec, (u_sec==1)?"":"s");
    }
    
    print_value ("Uptime","%s",uptimestr);
    print_value ("OS/Hardware","%s %s (%s)", VDstr("os","kernel"),
                 VDstr("os","version"), VDstr("os","arch"));
    Vdone("os");
    
    /* -------------------------------------------------------------*/
    print_hdr ("RESOURCES");
    print_value ("Processes","%llu (%llu running, %llu stuck)",
                             VDint("proc","total"),
                             VDint("proc","run"),
                             VDint("proc","stuck"));
    Vdone("proc");
    char cpubuf[128];
    sprintf (cpubuf, "%.2f (%.2f %%)", VAfrac("loadavg",0), Vfrac("pcpu"));
    Vdone("loadavg");
    
    char meter[32];
    strcpy (meter, "-[                      ]+");
    
    double pcpu = Vfrac("pcpu"); Vdone("pcpu");
    double level = 4.99;
    
    int pos = 2;
    while (level < 100.0 && pos < 22) {
        if (level < pcpu) meter[pos++] = '#';
        else meter[pos++] = ' ';
        level += 5.0;
    }
    
    print_value ("Load/CPU", "%-32s %s", cpubuf, meter);
    print_value ("Available RAM", "%.2f MB",
                 ((double)VDint("mem","total"))/1024.0);
    print_value ("Free RAM", "%.2f MB",
                 ((double)VDint("mem","free"))/1024.0);
    
    print_value ("Network in/out", "%i Kb/s (%i pps) / %i Kb/s (%i pps)",
                                   VDint("net","in_kbs"),
                                   VDint("net","in_pps"),
                                   VDint("net","out_kbs"),
                                   VDint("net","out_pps"));
    
    print_value ("Disk i/o", "%i rdops / %i wrops",
                 VDint("io","rdops"), VDint("io","wrops"));
    
    Vdone("mem");
    Vdone("net");
    Vdone("io");
    
    /* -------------------------------------------------------------*/
    print_hdr ("PROCESS LIST");
    
    const char *top_hdr[] = {"USER","PID","CPU","MEM","NAME",NULL};
    const char *top_fld[] = {"user","pid","pcpu","pmem","name",NULL};
    columnalign top_align[] = {CA_L, CA_R, CA_R, CA_R, CA_L, CA_NULL};
    vartype top_tp[] =
        {VAR_STR,VAR_INT,VAR_DOUBLE,VAR_DOUBLE,VAR_STR,VAR_NULL};
    int top_wid[] = {15, 7, 9, 9, 0, 0};
    int top_div[] = {0, 0, 0, 0, 0, 0};
    const char *top_suf[] = {"",""," %", " %", "", NULL};
    
    var *v_top = var_get_array_forkey (apires, "top");
    print_table (v_top, top_hdr, top_fld, 
                 top_align, top_tp, top_wid, top_suf, top_div);
    
    Vdone("top");
    /* -------------------------------------------------------------*/
    print_hdr ("STORAGE");
    
    const char *df_hdr[] = {"DEVICE","SIZE","FS","USED","MOUNTPOINT",NULL};
    const char *df_fld[] = {"device","size","fs","pused","mount",NULL};
    columnalign df_aln[] = {CA_L,CA_R,CA_L,CA_R,CA_L,CA_NULL};
    vartype df_tp[] = {VAR_STR,VAR_INT,VAR_STR,VAR_DOUBLE,VAR_STR,VAR_NULL};
    int df_wid[] = {12, 14, 6, 8, 0};
    int df_div[] = {0, (1024), 0, 0, 0, 0};
    const char *df_suf[] = {""," GB", "", " %", "", ""};
    
    var *v_df = var_get_array_forkey (apires, "df");
    print_table (v_df, df_hdr, df_fld,
                 df_aln, df_tp, df_wid, df_suf, df_div);
    
    Vdone("df");
    Vdone("badness");
    
    if (var_get_count (apires)) {
        print_hdr ("OTHER");
        print_values (apires, NULL);
    }
    
    printf ("---------------------------------------------"
            "-----------------------------------\n");

    var_free (apires);
    return 0;    
}

int cmd_bears (int argc, const char *argv[]) {
    var *v = api_get ("/obligatory-dancing-bears");
    puts (var_get_str_forkey (v,"bear"));
    var_free (v);
    return 0;
}