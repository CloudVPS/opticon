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

var *collect_meterdefs (uuid tenant, uuid host) {
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
    return conf;
}

/** GET / */
int cmd_list_tenants (req_context *ctx, req_arg *a, var *env, int *status) {
    int count = 0;
    var *v_tenants = var_get_array_forkey (env, "tenant");
    db *DB = localdb_create (OPTIONS.dbpath);
    uuid *list = db_list_tenants (DB, &count);
    
    for (int i=0; i<count; ++i) {
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
  *     name: "tenant name"
  * }
  */
int cmd_tenant_update (req_context *ctx, req_arg *a, var *env, int *status) {
    aeskey key;
    var *vopts = var_get_dict_forkey (ctx->bodyjson, "tenant");
    var *vkey = var_find_key (vopts, "key");
    const char *strvkey;
    char *strkey;
    if (vkey) strvkey = var_get_str (vkey);

    if ((!vkey) || strlen (strvkey) == 0) {
        return err_bad_request (ctx, a, env, status);
    }
    
    key = aeskey_from_base64 (strvkey);

    const char *sname = var_get_str_forkey (vopts, "name");
    if ((! sname) || strlen (sname) == 0) {
        return err_bad_request (ctx, a, env, status);
    }
    
    strkey = aeskey_to_base64 (key);
    var *outmeta = var_get_dict_forkey (env, "tenant");
    var_set_str_forkey (outmeta, "key", strkey);
    var_set_str_forkey (outmeta, "name", sname);
    
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        free (strkey);
        return err_not_found (ctx, a, env, status);
    }
    
    var *dbmeta = db_get_metadata (DB);
    var_set_str_forkey (dbmeta, "key", strkey);
    var_set_str_forkey (dbmeta, "name", sname);
    free (strkey);
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

/** POST /$TENANT/meta */
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
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    var *dbmeta = db_get_metadata (DB);
    var *dbmeta_meter = var_get_dict_forkey (dbmeta, "meter");
    var *env_meter = var_get_dict_forkey (env, "meter");
    var_copy (env_meter, dbmeta_meter);
    var_free (dbmeta);
    db_free (DB);
    *status = 200;
    return 1; 
}

static int set_meterid (char *meterid, req_arg *a) {
    const char *meterpart = a->argv[1];
    if (a->argc == 2) {
        if (strlen (meterpart) > 11) {
            return 0;
        }
        strcpy (meterid, meterpart);
    }
    else {
        const char *secondpart = a->argv[2];
        if (strlen (meterpart) + strlen (secondpart) > 10) {
            return 0;
        }
        sprintf (meterid, "%s/%s", meterpart, secondpart);
    }
    return 1;
}

/** POST /$TENANT/meter/$meterid[/$subid] */
int cmd_tenant_set_meter (req_context *ctx, req_arg *a, 
                          var *env, int *status) {
    if (a->argc < 2) return err_server_error (ctx, a, env, status);

    char meterid[16];
    if (! set_meterid (meterid, a)) err_bad_request (ctx, a, env, status);
    
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return err_not_found (ctx, a, env, status);
    }
    var *dbmeta = db_get_metadata (DB);
    var *dbmeta_meters = var_get_dict_forkey (dbmeta, "meter");
    var *dbmeta_meter = var_get_dict_forkey (dbmeta_meters, meterid);
    var *req_meters = var_get_dict_forkey (ctx->bodyjson, "meter");
    var *req_meter = var_get_dict_forkey (req_meters, meterid);
    const char *tstr;
    
    #define copystr(x) \
        if ((tstr = var_get_str_forkey (req_meter, x))) { \
            var_set_str_forkey (dbmeta_meter, x, tstr); \
        } else var_delete_key (dbmeta_meter, x)
    
    copystr ("type");
    copystr ("description");
    copystr ("unit");
    
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
    if (! set_meterid (meterid, a)) err_bad_request (ctx, a, env, status);
    
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
    var *res = collect_meterdefs (ctx->tenantid, uuidnil());
    if (! res) { return err_not_found (ctx, a, env, status); }
    
    var *env_watcher = var_get_dict_forkey (env, "watcher");
    var_copy (env_watcher, res);
    var_free (res);
    *status = 200;
    return 1;
}

int cmd_tenant_set_watcher (req_context *ctx, req_arg *a, 
                            var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

int cmd_tenant_delete_watcher (req_context *ctx, req_arg *a, 
                               var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

int cmd_tenant_list_hosts (req_context *ctx, req_arg *a, 
                            var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}
