#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <libopticon/datatypes.h>
#include <libopticon/aes.h>
#include <libopticon/codec_json.h>
#include <libopticon/ioport_file.h>
#include <libopticon/util.h>
#include <libopticon/var.h>
#include <libopticon/var_dump.h>
#include <libopticon/defaultmeters.h>
#include <libhttp/http.h>

#include "cmd.h"
#include "api.h"
#include "prettyprint.h"

/** The tenant-list command */
int cmd_tenant_list (int argc, const char *argv[]) {
    var *res = api_get ("/");
    if (OPTIONS.json) {
        var_dump (res, stdout);
    }
    else {
        printf ("UUID                                 Hosts  Name\n");
        printf ("---------------------------------------------"
                "-----------------------------------\n");

        var *res_tenant = var_get_array_forkey (res, "tenant");
        if (var_get_count (res_tenant)) {
            var *crsr = res_tenant->value.arr.first;
            while (crsr) {
                printf ("%s %5" PRIu64 "  %s\n",
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
    var_dump (meta, stdout);
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

/** The tenant-set-quota command */
int cmd_tenant_set_quota (int argc, const char *argv[]) {
    if (argc < 3) {
        fprintf (stderr, "%% Missing quota value\n");
        return 1;
    }
    
    uint64_t nquota = strtoull (argv[2], NULL, 10);
    if (! nquota) {
        fprintf (stderr, "%% Illegal quota value\n");
        return 1;
    }
    
    var *api_root = var_alloc();
    var *api_meta = var_get_dict_forkey (api_root, "tenant");
    var_set_int_forkey (api_meta, "quota", nquota);
    var *res = api_call ("PUT", api_root, "/%s", OPTIONS.tenant);
    var_free (api_root);
    var_free (res);
    return 0;
}

/** The tenant-get-quota command */
int cmd_tenant_get_quota (int argc, const char *argv[]) {
    if (argc < 2) {
        fprintf (stderr, "%% Syntax error");
        return 1;
    }
    
    var *meta = api_get ("/%s/quota", OPTIONS.tenant);
    if (! meta) return 1;
    printf ("Tenant storage quota: %llu MB\n",
            var_get_int_forkey (meta, "quota"));
    printf ("Current usage: %.2f %%\n",
            var_get_double_forkey (meta, "usage"));
    var_free (meta);
    return 0;
}

/** The tenant-get-summary command */
int cmd_tenant_get_summary (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }

    var *summ = api_get ("/%s/summary", OPTIONS.tenant);
    var_dump (summ, stdout);
    var_free (summ);
    return 0;
}

/** The host-overview command */
int cmd_host_overview (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }

    var *ov = api_get ("/%s/host/overview", OPTIONS.tenant);
    if (! var_get_count (ov)) return 0;

    printf ("Name                            Status     "
            "Load  Net i/o      CPU\n");
    printf ("---------------------------------------------"
            "-----------------------------------\n");
    
    var *ov_dict = var_get_dict_forkey (ov, "overview");
    if (! var_get_count (ov_dict)) return 0;
    var *crsr = ov_dict->value.arr.first;
    while (crsr) {
        const char *hname = var_get_str_forkey (crsr, "hostname");
        if (! hname) hname = "";
        char shortname[32];
        strncpy (shortname, hname, 31);
        shortname[31] = 0;
        const char *hstat = var_get_str_forkey (crsr, "status");
        if (! hstat ) hstat = "UNSET";
        double load = var_get_double_forkey (crsr, "loadavg");
        uint64_t netio = var_get_int_forkey (crsr, "net/in_kbs");
        netio += var_get_int_forkey (crsr, "net/out_kbs");
        double cpu = var_get_double_forkey (crsr, "pcpu");
        int rcpu = (cpu+5.0) / 10;
        printf ("%-31s %-8s %6.2f %8" PRIu64 " %6.2f %% -[",
                shortname, hstat, load, netio, cpu);
        for (int i=0; i<10; i++) {
            if (i< rcpu) printf ("#");
            else printf (" ");
        }
        printf ("]+\n");
        crsr = crsr->next;
    }
    var_free (ov);
    printf ("---------------------------------------------"
            "-----------------------------------\n");
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
    if (OPTIONS.json) {
        var_dump (apires, stdout);
    }
    else {
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
            sprintf (valst, "%" PRIu64, var_get_int (val));
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
    
    var *apires;
    
    if (OPTIONS.host[0]) {
        apires = api_get ("/%s/host/%s/watcher", OPTIONS.tenant, OPTIONS.host);
    }
    else {
        apires = api_get ("/%s/watcher", OPTIONS.tenant);
    }
    if (OPTIONS.json) {
        var_dump (apires, stdout);
        var_free (apires);
        return 0;
    }

    printf ("From     Meter        Trigger   Match                  "
            "Value             Weight\n"
            "-------------------------------------------------------"
            "-------------------------\n");

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
    
    if (OPTIONS.json) {
        var_dump (apires, stdout);
        var_free (apires);
        return 0;
    }
    
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

            printf ("%s %4" PRIu64 " %s %s  %s\n",
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

/** The delete-host command */
int cmd_remove_host (int argc, const char *argv[]) {
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    if (OPTIONS.host[0] == 0) {
        fprintf (stderr, "%% No hostid provided\n");
        return 1;
    }
    
    var *p = var_alloc();
    var *apires = api_call ("DELETE", p, "/%s/host/%s",
                            OPTIONS.tenant, OPTIONS.host);
    var_free (p);
    var_free (apires);
    return 0;
}

/** The get-record command */
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
        var_dump (apires, stdout);
        var_free (apires);
        return 0;
    }
    
    #define Arr(x) var_get_array_forkey(apires,x)
    #define Vint(x) var_get_int_forkey(apires,x)
    #define Vstr(x) var_get_str_forkey(apires,x)
    #define Vfrac(x) var_get_double_forkey(apires,x)
    #define VDint(x,y) var_get_int_forkey(var_get_dict_forkey(apires,x),y)
    #define VDstr(x,y) var_get_str_forkey(var_get_dict_forkey(apires,x),y)
    #define VDfrac(x,y) var_get_double_forkey(var_get_dict_forkey(apires,x),y)
    #define VAfrac(x,y) var_get_double_atindex(var_get_array_forkey(apires,x),y)
    #define Vdone(x) var_delete_key(apires,x)
    /* -------------------------------------------------------------*/
    print_hdr ("HOST");
    print_value ("UUID", "%s", OPTIONS.host);
    print_value ("Hostname", "%s", Vstr("hostname"));
    print_value ("Address", "%s", VDstr("agent","ip"));
    print_value ("Status", "\033[1m%s\033[0m", Vstr("status"));
    
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
        sprintf (uptimestr, "%" PRIu64 " day%s, %" PRIu64 ":%02" PRIu64 ":%02" PRIu64 "", u_days,
                 (u_days==1)?"":"s", u_hours, u_mins, u_sec);
    }
    else if (u_hours) {
        sprintf (uptimestr, "%" PRIu64 ":%02" PRIu64 ":%02" PRIu64, u_hours, u_mins, u_sec);
    }
    else {
        sprintf (uptimestr, "%" PRIu64 " minute%s, %" PRIu64 " second%s",
                 u_mins, (u_mins==1)?"":"s", u_sec, (u_sec==1)?"":"s");
    }
    
    print_value ("Uptime","%s",uptimestr);
    print_value ("OS/Hardware","%s %s (%s)", VDstr("os","kernel"),
                 VDstr("os","version"), VDstr("os","arch"));
    const char *dist = VDstr("os","distro");
    if (dist) print_value ("Distribution", "%s", dist);
    Vdone("os");
    
    /* -------------------------------------------------------------*/
    print_hdr ("RESOURCES");
    print_value ("Processes","\033[1m%" PRIu64 "\033[0m "
                             "(\033[1m%" PRIu64 "\033[0m running, "
                             "\033[1m%" PRIu64 "\033[0m stuck)",
                             VDint("proc","total"),
                             VDint("proc","run"),
                             VDint("proc","stuck"));
    Vdone("proc");
 
     print_value ("Load Average", "\033[1m%6.2f\033[0m / "
                                  "\033[1m%6.2f\033[0m / "
                                  "\033[1m%6.2f\033[0m",
                 VAfrac ("loadavg",0), VAfrac ("loadavg", 1),
                 VAfrac ("loadavg",2));
    Vdone ("loadavg");

    char cpubuf[128];
    sprintf (cpubuf, "\033[1m%6.2f \033[0m%%", Vfrac("pcpu"));
    
    char meter[32];
    strcpy (meter, "-[                      ]+");
    
    double iowait = VDfrac("io","pwait");
    double pcpu = Vfrac("pcpu"); Vdone("pcpu");
    double level = 4.5;
    
    int pos = 2;
    while (level < 100.0 && pos < 22) {
        if (level < pcpu) meter[pos++] = '#';
        else meter[pos++] = ' ';
        level += 4.5;
    }
    
    
    print_value ("CPU", "%-40s %s", cpubuf, meter);
    if (iowait>0.001) print_value ("CPU iowait", 
                                   "\033[1m%6.2f %%\033[0m", iowait);
    print_value ("Available RAM", "\033[1m%.2f\033[0m MB",
                 ((double)VDint("mem","total"))/1024.0);
    print_value ("Free RAM", "\033[1m%.2f\033[0m MB",
                 ((double)VDint("mem","free"))/1024.0);
    
    print_value ("Network in/out", "\033[1m%i\033[0m Kb/s "
                                   "(\033[1m%i\033[0m pps) / "
                                   "\033[1m%i\033[0m Kb/s "
                                   "(\033[1m%i\033[0m pps)",
                                   VDint("net","in_kbs"),
                                   VDint("net","in_pps"),
                                   VDint("net","out_kbs"),
                                   VDint("net","out_pps"));
    
    print_value ("Disk i/o", "\033[1m%i\033[0m rdops / "
                             "\033[1m%i\033[0m wrops",
                 VDint("io","rdops"), VDint("io","wrops"));
    
    Vdone("mem");
    Vdone("net");
    Vdone("io");
    Vdone("badness");

    print_values (apires, NULL);
    
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
    
    /*print_generic_table (v_df);*/
    
    print_table (v_df, df_hdr, df_fld,
                 df_aln, df_tp, df_wid, df_suf, df_div);
    
    Vdone("df");
    
    /** Print any remaining table data */
    print_tables (apires);
        
    printf ("---------------------------------------------"
            "-----------------------------------\n");

    var_free (apires);
    print_done();
    return 0;    
}

/** The session-list command [admin] */
int cmd_session_list (int argc, const char *argv[]) {
    var *v = api_get ("/session");

    if (OPTIONS.json) {
        var_dump (v, stdout);
        var_free (v);
        return 0;
    }
    
    printf ("---------------------------------------------"
            "-----------------------------------\n");
    printf ("Session ID         Sender"
            "                                 Last Refresh\n");
    
    var *v_session = var_get_array_forkey (v, "session");
    var *crsr = v_session->value.arr.first;
    while (crsr) {
        printf ("%08x-%08x %-39s %s\n",
                (uint32_t) (var_get_int_forkey (crsr, "sessid")),
                (uint32_t) (var_get_int_forkey (crsr, "addr")),
                var_get_str_forkey (crsr, "remote"),
                var_get_str_forkey (crsr, "lastcycle"));
        crsr = crsr->next;
    }

    printf ("---------------------------------------------"
            "-----------------------------------\n");

    var_free (v);
    return 0;
}

/** The dancing-bears command */
int cmd_bears (int argc, const char *argv[]) {
    var *v = api_get ("/obligatory-dancing-bears");
    puts (var_get_str_forkey (v,"bear"));
    var_free (v);
    return 0;
}
