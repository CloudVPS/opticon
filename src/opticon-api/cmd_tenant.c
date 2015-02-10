#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <libopticon/util.h>
#include <libopticon/datatypes.h>
#include <libopticon/defaultmeters.h>
#include <libopticon/aes.h>
#include <microhttpd.h>
#include "req_context.h"
#include "cmd.h"
#include "options.h"

/** Build up a summarized collection of meter information bound to
  * a specific tenant or host.
  * \param tenant The tenant uuid
  * \param host The host uuid (can be uuidnil())
  * \param watchonly If 1, collect watcher data, not meter data
  * \return var object ready for transmission.
  */
var *collect_meterdefs (uuid tenant, uuid host, int watchonly) {
    char tenantstr[40];
    uuid2str (tenant, tenantstr);
    char hoststr[40];
    uuid2str (host, hoststr);
    
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, tenant, NULL)) {
        log_error ("(collect_meterdefs) could not open %s", tenantstr);
        db_free (DB);
        return NULL;
    }
    
    const char *triggers[3] = {"warning","alert","critical"};
    
    var *tenantmeta = db_get_metadata (DB);
    if (!tenantmeta) tenantmeta = var_alloc();
    var *hostmeta = db_get_hostmeta (DB, host);
    if (!hostmeta) hostmeta = var_alloc();
    
    var *conf = var_alloc();
    var *cmeters = var_get_dict_forkey (conf, watchonly?"watcher":"meter");
    var_copy (cmeters, OPTIONS.mconf);
    var *tmeters = var_get_dict_forkey (tenantmeta, "meter");
    var *hmeters = var_get_dict_forkey (hostmeta, "meter");
    const char *tstr;
    var *tvar;
    
    var *tc = tmeters->value.arr.first;
    while (tc) {
        tvar = var_find_key (cmeters, tc->id);
        var *cc = tvar;
        if (! cc) cc = var_get_dict_forkey (cmeters, tc->id);
        tstr = var_get_str_forkey (tc, "type");
        if (tstr) var_set_str_forkey (cc, "type", tstr);
        if (! watchonly) {
            tstr = var_get_str_forkey (tc, "description");
            if (tstr) var_set_str_forkey (cc, "description", tstr);
            tstr = var_get_str_forkey (tc, "unit");
            if (tstr) var_set_str_forkey (cc, "unit", tstr);
            if (! tvar) var_set_str_forkey (cc, "origin", "tenant");
        }
        
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
    while (watchonly && hc) {
        var *cc = var_get_dict_forkey (cmeters, hc->id);
        tstr = var_get_str_forkey (hc, "type");
        if (! tstr) {
            log_error ("(collect_meterdefs) No type set for meter "
                       "%s in watchers for host %s/%s",
                       hc->id, tenantstr, hoststr);
            return NULL;
        }
        const char *origtype = var_get_str_forkey (cc, "type");
        if (strcmp (tstr, origtype) != 0) {
            log_error ("(collect_meterdefs) Type %s set for "
                       "watcher %s in host %s/%s doesn't match "
                       "definition %s", tstr, hc->id,
                       tenantstr, hoststr, origtype);
            return NULL;
        }
        
        for (int tr=0; tr<3; ++tr) {
            tvar = var_find_key (hc, triggers[tr]);
            if (tvar) {
                var *cdef = var_find_key (cc, triggers[tr]);
                var *val = var_find_key (tvar, "val");
                var *cval = var_find_key (cdef, "val");
                if (! cval) {
                    log_error ("(collect_meterdefs) Ill-defined "
                               "value in configuration of watcher "
                               "%s at level %s", hc->id,
                               triggers[tr]);
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
    
    var *crsr = cmeters->value.arr.first;
    var *ncrsr;
    while (crsr) {
        ncrsr = crsr->next;
        if (watchonly) {
            if ( (! var_find_key (crsr, "warning")) &&
                 (! var_find_key (crsr, "alert")) &&
                 (! var_find_key (crsr, "critical")) ) {
                var_delete_key (cmeters, crsr->id);
            }
            else {
                var_delete_key (crsr, "description");
                var_delete_key (crsr, "unit");
            }
        }
        else {
            var_delete_key (crsr, "warning");
            var_delete_key (crsr, "alert");
            var_delete_key (crsr, "critical");
        }
        crsr = ncrsr;
    }
    
    return conf;
}

/** GET / */
int cmd_list_tenants (req_context *ctx, req_arg *a, var *env, int *status) {
    int count = 0;
    var *v_tenants = var_get_array_forkey (env, "tenant");
    db *DB = localdb_create (OPTIONS.dbpath);
    uuid *list = db_list_tenants (DB, &count);
    
    for (int i=0; i<count; ++i) {
        /* Filter only tenants the token has access to */
        if (ctx->userlevel != AUTH_ADMIN) {
            if (ctx->auth_tenantcount == 0) continue;
            int found = 0;
            for (int j=0; j<ctx->auth_tenantcount; ++j) {
                if (uuidcmp (ctx->auth_tenants[j], list[i])) {
                    found = 1;
                    break;
                }
            }
            if (! found) continue;
        }
        int cnt = 0;
        var *meta = NULL;
        const char *tenantname = NULL;
        
        if (db_open (DB, list[i], NULL)) {
            uuid *llist = db_list_hosts (DB, &cnt);
            meta = db_get_metadata (DB);
            if (llist) free(llist);
            db_close (DB);
        }
        
        if (meta) tenantname = var_get_str_forkey (meta, "name");
        if (!tenantname) tenantname = "";
        var *node = var_add_dict (v_tenants);
        var_set_uuid_forkey (node, "id", list[i]);
        var_set_str_forkey (node, "name", tenantname);
        var_set_int_forkey (node, "hostcount", cnt);
    }
    
    free (list);
    db_free (DB);
    *status = 200;
    return 1;
}

/** GET /token */
int cmd_token (req_context *ctx, req_arg *a, var *env, int *status) {
    var *env_token = var_get_dict_forkey (env, "token");
    switch (ctx->userlevel) {
        case AUTH_GUEST:
            var_set_str_forkey (env_token, "userlevel", "AUTH_GUEST");
            break;
        
        case AUTH_USER:
            var_set_str_forkey (env_token, "userlevel", "AUTH_USER");
            break;
        
        case AUTH_ADMIN:
            var_set_str_forkey (env_token, "userlevel", "AUTH_ADMIN");
            break;
    }
    *status = 200;
    return 1;
}

/** GET /$TENANT */
int cmd_tenant_get (req_context *ctx, req_arg *a, var *env, int *status) {
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    var *v_meta = var_get_dict_forkey (env, "tenant");
    var *meta = db_get_metadata (DB);
    if (! meta) meta = var_alloc();
    var_copy (v_meta, meta);
    var_delete_key (v_meta, "meter");
    var_free (meta);
    db_free (DB);
    *status = 200;
    return 1;
}

/** POST /$TENANT
  * tenant: {
  *     key: "base64", # optional
  *     name: "tenant name"
  * }
  */
int cmd_tenant_create (req_context *ctx, req_arg *a, var *env, int *status) {
    aeskey key;
    var *vopts = var_get_dict_forkey (ctx->bodyjson, "tenant");
    var *vkey = var_find_key (vopts, "key");
    char *strkey;
    if (vkey) key = aeskey_from_base64 (var_get_str (vkey));
    else key = aeskey_create();
    
    const char *sname = var_get_str_forkey (vopts, "name");
    if ((! sname) || strlen (sname) == 0) {
        return err_bad_request (ctx, a, env, status);
    }
    
    strkey = aeskey_to_base64 (key);
    var *outmeta = var_get_dict_forkey (env, "tenant");
    var_set_str_forkey (outmeta, "key", strkey);
    var_set_str_forkey (outmeta, "name", sname);
    free (strkey);
    
    db *DB = localdb_create (OPTIONS.dbpath);
    
    /* Reject existing tenants */
    if (db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_conflict (ctx, a, env, status);
    }
    
    if (! db_create_tenant (DB, ctx->tenantid, outmeta)) {
        db_free (DB);
        return err_server_error (ctx, a, env, status);
    }
    
    db_free (DB);
    *status = 200;
    return 1;
}

/** PUT /$TENANT
  * tenant: {
  *     key: "base64",
  *     name: "tenant name" # optional (needs admin)
  * }
  */
int cmd_tenant_update (req_context *ctx, req_arg *a, var *env, int *status) {
    aeskey key;
    var *vopts = var_get_dict_forkey (ctx->bodyjson, "tenant");
    var *vkey = var_find_key (vopts, "key");
    const char *strvkey = NULL;
    char *strkey = NULL;
    uint64_t iquota = var_get_int_forkey (vopts, "quota");
    if (vkey) strvkey = var_get_str (vkey);

    if (vkey && strlen (strvkey) == 0) {
        return err_bad_request (ctx, a, env, status);
    }
    
    const char *sname = var_get_str_forkey (vopts, "name");
    
    if (sname && (ctx->userlevel != AUTH_ADMIN)) {
        return err_not_allowed (ctx, a, env, status);
    }
    
    if (iquota && (ctx->userlevel != AUTH_ADMIN)) {
        return err_not_allowed (ctx, a, env, status);
    }

    var *outmeta = var_get_dict_forkey (env, "tenant");

    if (strvkey) {    
        key = aeskey_from_base64 (strvkey);
        strkey = aeskey_to_base64 (key);
        var_set_str_forkey (outmeta, "key", strkey);
    }
    
    if (sname) var_set_str_forkey (outmeta, "name", sname);
    if (iquota) var_set_int_forkey (outmeta, "quota", iquota);
    
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        free (strkey);
        return err_not_found (ctx, a, env, status);
    }
    
    log_info ("Update tenant with new quota %llu", iquota);
    
    var *dbmeta = db_get_metadata (DB);
    if (strkey) var_set_str_forkey (dbmeta, "key", strkey);
    if (sname) var_set_str_forkey (dbmeta, "name", sname);
    if (iquota) var_set_int_forkey (dbmeta, "quota", iquota);
    if (strkey) free (strkey);
    db_set_metadata (DB, dbmeta);
    *status = 200;
    return 1;
}

/** DELETE /$TENANT */
int cmd_tenant_delete (req_context *ctx, req_arg *a, var *env, int *status) {
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    db_close (DB);
    db_remove_tenant (DB, ctx->tenantid);
    var_set_uuid_forkey (env, "deleted", ctx->tenantid);
    *status = 200;
    return 1;
}

/** GET /$TENANT/meta */
int cmd_tenant_get_meta (req_context *ctx, req_arg *a,
                         var *env, int *status) {
    
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    var *v_meta = var_get_dict_forkey (env, "metadata");
    var *meta = db_get_metadata (DB);
    if (! meta) meta = var_alloc();
    var *v_dbusermeta = var_get_dict_forkey (meta, "meta");
    var_copy (v_meta, v_dbusermeta);
    var_free (meta);
    db_free (DB);
    *status = 200;
    return 1;
}

/** GET /$TENANT/summary */
int cmd_tenant_get_summary (req_context *ctx, req_arg *a,
                            var *env, int *status) {
    
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    var *v_meta = var_get_dict_forkey (env, "summary");
    var *summ = db_get_summary (DB);
    if (! summ) summ = var_alloc();
    var_copy (v_meta, summ);
    var_free (summ);
    db_free (DB);
    *status = 200;
    return 1;
}

/** POST /$TENANT/meta 
  * metadata: {
  *    key: value, ...
  * }
  */
int cmd_tenant_set_meta (req_context *ctx, req_arg *a, 
                         var *env, int *status) {

    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    var *v_meta = var_get_dict_forkey (ctx->bodyjson, "metadata");
    var *meta = db_get_metadata (DB);
    var *v_dbusermeta = var_get_dict_forkey (meta, "meta");
    var_copy (v_dbusermeta, v_meta);
    db_set_metadata (DB, meta);
    var_copy (var_get_dict_forkey (env, "metadata"), v_meta);
    var_free (meta);
    db_free (DB);
    *status = 200;
    return 1;
}

/** GET /$TENANT/meter */
int cmd_tenant_list_meters (req_context *ctx, req_arg *a, 
                            var *env, int *status) {
    var *res = collect_meterdefs (ctx->tenantid, uuidnil(), 0);
    if (! res) { return err_not_found (ctx, a, env, status); }
    
    var_copy (env, res);
    var_free (res);
    *status = 200;
    return 1;
}

/** Utility function for getting a meterid string out of the
  * argument list. Because meterids can contain slashes, the
  * actual id is sometimes 1, sometimes 2 arguments long.
  * This function combines them and puts them into a single
  * string.
  * \param meterid String to write to (should be > size 12)
  * \param a The request argument list
  * \param pos The position of the first half of the id in the
  *            argument list.
  * \return 1 on success, 0 if a valid meterid couldn't be
  *         extracted.
  */
int set_meterid (char *meterid, req_arg *a, int pos) {
    const char *meterpart = a->argv[pos];
    if (a->argc == (pos+1)) {
        if (strlen (meterpart) > 11) {
            return 0;
        }
        strcpy (meterid, meterpart);
    }
    else {
        const char *secondpart = a->argv[pos+1];
        if (strlen (meterpart) + strlen (secondpart) > 10) {
            return 0;
        }
        sprintf (meterid, "%s/%s", meterpart, secondpart);
    }
    return 1;
}

/** POST /$TENANT/meter/$meterid[/$subid] 
  * meter: {
  *     type: "integer",
  *     description: "Description text", # optional
  *     unit: "storpels/s" # optional
  * }
  */
int cmd_tenant_set_meter (req_context *ctx, req_arg *a, 
                          var *env, int *status) {
    if (a->argc < 2) return err_server_error (ctx, a, env, status);

    char meterid[16];
    if (! set_meterid (meterid, a, 1)) err_bad_request (ctx, a, env, status);
    
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    var *dbmeta = db_get_metadata (DB);
    var *dbmeta_meters = var_get_dict_forkey (dbmeta, "meter");
    var *dbmeta_meter = var_get_dict_forkey (dbmeta_meters, meterid);
    var *req_meter = var_get_dict_forkey (ctx->bodyjson, "meter");
    const char *tstr;
    
    /* validate type */
    tstr = var_get_str_forkey (req_meter, "type");
    if (!tstr || (strcmp (tstr, "integer") &&
                  strcmp (tstr, "frac") &&
                  strcmp (tstr, "table") &&
                  strcmp (tstr, "string"))) {
        var_free (dbmeta);
        db_free (DB);
        return err_bad_request (ctx, a, env, status);
    }
    
    #define copystr(x) \
        if ((tstr = var_get_str_forkey (req_meter, x))) { \
            if (strlen (tstr) > 128) { \
                var_free (dbmeta); \
                db_free (DB); \
                *status = 400; \
                return err_generic (env, "String too long"); \
            } \
            var_set_str_forkey (dbmeta_meter, x, tstr); \
        } else var_delete_key (dbmeta_meter, x)
    
    copystr ("type");
    copystr ("description");
    if (strcmp (tstr, "table")) { copystr ("unit"); }
    
    #undef copystr
    
    db_set_metadata (DB, dbmeta);
    var *env_meters = var_get_dict_forkey (env, "meter");
    var *env_meter = var_get_dict_forkey (env_meters, meterid);
    var_copy (env_meter, dbmeta_meter);
    var_free (dbmeta);
    db_free (DB);
    *status = 200;
    return 1;
}

/** DELETE /$TENANT/meter/$meterid[/$subid] */
int cmd_tenant_delete_meter (req_context *ctx, req_arg *a, 
                             var *env, int *status) {
    if (a->argc < 2) return err_server_error (ctx, a, env, status);

    char meterid[16];
    if (! set_meterid (meterid, a, 1)) err_bad_request (ctx, a, env, status);
    
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    var *dbmeta = db_get_metadata (DB);
    var *dbmeta_meters = var_get_dict_forkey (dbmeta, "meter");
    var_delete_key (dbmeta_meters, meterid);
    db_set_metadata (DB, dbmeta);
    var *env_meter = var_get_dict_forkey (env, "meter");
    var_set_str_forkey (env_meter, "deleted", meterid);
    var_free (dbmeta);
    db_free (DB);
    *status = 200;
    return 1;
}

/** GET /$TENANT/watcher */
int cmd_tenant_list_watchers (req_context *ctx, req_arg *a, 
                            var *env, int *status) {
    var *res = collect_meterdefs (ctx->tenantid, uuidnil(), 1);
    if (! res) { return err_not_found (ctx, a, env, status); }
    
    var_copy (env, res);
    var_free (res);
    *status = 200;
    return 1;
}

/** POST /$TENANT/watcher/$METERID
  * watcher {
  *     warning { cmp: gt, value: 10, weight:1.0 } # optional
  * }
  */
int cmd_tenant_set_watcher (req_context *ctx, req_arg *a, 
                            var *env, int *status) {
    if (a->argc < 2) return err_server_error (ctx, a, env, status);

    char meterid[16];
    if (! set_meterid (meterid, a, 1)) err_bad_request (ctx, a, env, status);
    
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    var *ctxwatcher = var_get_dict_forkey (ctx->bodyjson, "watcher");
    var *dbmeta = db_get_metadata (DB);
    var *dbmeta_meters = var_get_dict_forkey (dbmeta, "meter");
    var *dbmeta_meter = var_get_dict_forkey (dbmeta_meters, meterid);
    const char *meter_type = var_get_str_forkey (dbmeta_meter, "type");
    if (meter_type && strcmp (meter_type, "table") == 0) {
        var_free (dbmeta);
        db_free (DB);
        *status = 400;
        return err_generic (env, "Cannot set watcher for table root");
    }
    
    var *tv;

    tv = var_find_key (ctxwatcher, "warning");
    if (tv) var_copy (var_get_dict_forkey (dbmeta_meter, "warning"), tv);
    tv = var_find_key (ctxwatcher, "alert");
    if (tv) var_copy (var_get_dict_forkey (dbmeta_meter, "alert"), tv);
    tv = var_find_key (ctxwatcher, "critical");
    if (tv) var_copy (var_get_dict_forkey (dbmeta_meter, "critical"), tv);
    
    db_set_metadata (DB, dbmeta);
    var *env_watcher = var_get_dict_forkey (env, "watcher");
    var_copy (env_watcher, ctxwatcher);
    var_free (dbmeta);
    db_free (DB);
    *status = 200;
    return 1;
}

/** DELETE /$TENANT/watcher/$METERID */
int cmd_tenant_delete_watcher (req_context *ctx, req_arg *a, 
                               var *env, int *status) {
    if (a->argc < 2) return err_server_error (ctx, a, env, status);

    char meterid[16];
    if (! set_meterid (meterid, a, 1)) err_bad_request (ctx, a, env, status);
    
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    var *dbmeta = db_get_metadata (DB);
    var *dbmeta_meters = var_get_dict_forkey (dbmeta, "meter");
    var *dbmeta_meter = var_get_dict_forkey (dbmeta_meters, meterid);
    var_delete_key (dbmeta_meter, "warning");
    var_delete_key (dbmeta_meter, "alert");
    var_delete_key (dbmeta_meter, "critical");
    db_set_metadata (DB, dbmeta);
    var *env_watcher = var_get_dict_forkey (env, "watcher");
    var_set_str_forkey (env_watcher, "deleted", meterid);
    var_free (dbmeta);
    db_free (DB);
    *status = 200;
    return 1;
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

/** GET /$TENANT/host */
int cmd_tenant_list_hosts (req_context *ctx, req_arg *a, 
                            var *env, int *status) {
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    
    var *env_hosts = var_get_array_forkey (env, "host");
    
    char uuidstr[40];
    char *str_early;
    char *str_late;
    
    usage_info usage;
    int count;
    uuid *list = db_list_hosts (DB, &count);
    for (int i=0; i<count; ++i) {
        uuid2str (list[i], uuidstr);
        if (db_get_usage (DB, &usage, list[i])) {
            str_early = timfmt (usage.earliest, 1);
            str_late = timfmt (usage.last, 1);
            var *crsr = var_add_dict (env_hosts);
            var_set_str_forkey (crsr, "id", uuidstr);
            var_set_int_forkey (crsr, "usage", usage.bytes);
            var_set_str_forkey (crsr, "start", str_early);
            var_set_str_forkey (crsr, "end", str_late);
            free (str_early);
            free (str_late);
        }
    }
    free (list);
    *status = 200;
    return 1;
}
