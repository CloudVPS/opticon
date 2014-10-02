#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <libopticon/util.h>
#include <libopticon/datatypes.h>
#include <libopticon/codec_json.h>
#include <libopticon/dump.h>
#include <microhttpd.h>
#include "req_context.h"
#include "cmd.h"
#include "options.h"

static codec *JSONCODEC = NULL;

/** GET /$TENANT/host/$HOST */
int cmd_host_get (req_context *ctx, req_arg *a, ioport *outio, int *status) {
    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return 0;
    }
    if (!JSONCODEC) JSONCODEC= codec_create_json();
    host *h = host_alloc();
    time_t tnow = time (NULL);
    
    h->uuid = ctx->hostid;
    if (db_get_record (DB, tnow, h)) {
        codec_encode_host (JSONCODEC, outio, h);
        *status = 200;
        db_free (DB);
        host_delete (h);
        return 1;
    }
    
    host_delete (h);
    db_free (DB);
    
    var *err = var_alloc();
    var_set_str_forkey (err, "error", "No current record found for host");
    write_var (err, outio);
    var_free (err);
    *status = 404;
    return 1;
}

/** GET /$TENANT/host/$HOST/watcher */
int cmd_host_list_watchers (req_context *ctx, req_arg *a, 
                            var *env, int *status) {
    var *res = collect_meterdefs (ctx->tenantid, ctx->hostid, 1);
    if (! res) { return err_not_found (ctx, a, env, status); }
    
    var_copy (env, res);
    var_free (res);
    *status = 200;
    return 1;
}

/** POST /$TENANT/host/$HOST/watcher/$METERID 
  * watcher {
  *     warning { val: x, weight: 1.0 } # optional
  *     alert { val: x, weight: 1.0 } # optional
  *     critical { val: x, weight: 1.0 } #optional
  * }
  */
int cmd_host_set_watcher (req_context *ctx, req_arg *a, 
                          var *env, int *status) {
    if (a->argc < 3) return err_server_error (ctx, a, env, status);

    char meterid[16];
    if (! set_meterid (meterid, a, 2)) err_bad_request (ctx, a, env, status);

    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return 0;
    }
    
    var *def = collect_meterdefs (ctx->tenantid, uuidnil(), 0);
    var *def_meters = var_get_dict_forkey (def, "meter");
    var *def_meter = var_get_dict_forkey (def_meters, meterid);
    var *hmeta = db_get_hostmeta (DB, ctx->hostid);
    var *hmeta_meters = var_get_dict_forkey (hmeta, "meter");
    var *hmeta_meter = var_get_dict_forkey (hmeta_meters, meterid);
    
    /* Copy the type from parent information */
    const char *tp = var_get_str_forkey (def_meter, "type");
    if (! tp) tp = "integer";
    var_set_str_forkey (hmeta_meter, "type", tp);
    
    var *in_watcher = var_get_dict_forkey (ctx->bodyjson, "watcher");
    
    const char *triggers[3] = {"warning","alert","critical"};
    
    /* Go over each trigger level, only copy what's actually there.
       Also only copy values relevant to a watcher */
    for (int i=0;i<3;++i) {
        var *tvar;
        tvar = var_find_key (in_watcher, triggers[i]);
        if (tvar) {
            var *w = var_get_dict_forkey (hmeta_meter, triggers[i]);
            var *nval = var_find_key (tvar, "val");
            if (! nval) {
                var_free (def);
                var_free (hmeta);
                db_free (DB);
                return err_bad_request (ctx, a, env, status);
            }
            var *wval = var_find_key (w, "val");
            if (! wval) {
                wval = var_alloc();
                strcpy (wval->id, "val");
                var_copy (wval, nval);
                var_link (wval, w);
            }
            else var_copy (wval, nval);
            
            /* Copy weight or set default */
            double weight = var_get_double_forkey (tvar, "weight");
            if (weight < 0.0001) weight = 1.0;
            var_set_double_forkey (wval, "weight", weight);
        }
    }
    
    dump_var (hmeta, stdout);
    
    db_set_hostmeta (DB, ctx->hostid, hmeta);
    var *envwatcher = var_get_dict_forkey (env, "watcher");
    var_copy (envwatcher, hmeta_meter);
    db_free (DB);
    var_free (def);
    var_free (hmeta);
    *status = 200;
    return 1;
}

/** DELETE /$TENANT/host/$HOST/watcher/$METERID */
int cmd_host_delete_watcher (req_context *ctx, req_arg *a, 
                             var *env, int *status) {
    if (a->argc < 3) return err_server_error (ctx, a, env, status);

    char meterid[16];
    if (! set_meterid (meterid, a, 2)) err_bad_request (ctx, a, env, status);

    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return 0;
    }

    var *hmeta = db_get_hostmeta (DB, ctx->hostid);
    var *hmeta_meters = var_get_dict_forkey (hmeta, "meter");
    var *hmeta_meter = var_get_dict_forkey (hmeta_meters, meterid);

    var_delete_key (hmeta_meter, "warning");
    var_delete_key (hmeta_meter, "alert");
    var_delete_key (hmeta_meter, "critical");
    
    db_set_hostmeta (DB, ctx->hostid, hmeta);
    var_free (hmeta);
    db_free (DB);
    var *env_meter = var_get_dict_forkey (env, "meter");
    var_set_str_forkey (env_meter, "deleted", meterid);
    *status = 200;
    return 1;
}

int cmd_host_get_range (req_context *ctx, req_arg *a, 
                        var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

int cmd_host_get_time (req_context *ctx, req_arg *a, 
                        ioport *outio, int *status) {

    db *DB = localdb_create (OPTIONS.dbpath);
    if (! db_open (DB, ctx->tenantid, NULL)) {
        db_free (DB);
        return 0;
    }
    if (!JSONCODEC) JSONCODEC= codec_create_json();
    host *h = host_alloc();
    time_t tnow = time (NULL);
    
    h->uuid = ctx->hostid;
    if (db_get_record (DB, tnow, h)) {
        codec_encode_host (JSONCODEC, outio, h);
        *status = 200;
        db_free (DB);
        host_delete (h);
        return 1;
    }
    
    host_delete (h);
    db_free (DB);
    return 0;
}

int cmd_dancing_bears (req_context *ctx, req_arg *a, var *env, int *status) {
    const char *B = "    _--_     _--_    _--_     _--_     "
                    "_--_     _--_     _--_     _--_\n"
                    "   (    )~~~(    )  (    )~~~(    )   ("
                    "    )~~~(    )   (    )~~~(    )\n"
                    "    \\           /    \\           /   "
                    "  \\           /     \\           /\n"
                    "     (  ' _ `  )      (  ' _ `  )      "
                    " (  ' _ `  )       (  ' _ `  )\n"
                    "      \\       /        \\       /     "
                    "    \\       /         \\       /\n"
                    "    .__( `-' )          ( `-' )        "
                    "   ( `-' )        .__( `-' )  ___\n"
                    "   / !  `---' \\      _--'`---_        "
                    "  .--`---'\\       /   /`---'`-'   \\\n"
                    "  /  \\         !    /         \\___   "
                    "  /        _>\\    /   /          ._/   __\n"
                    " !   /\\        )   /   /       !  \\  "
                    " /  /-___-'   ) /'   /.-----\\___/     /  )\n"
                    " !   !_\\       ). (   <        !__/ /'"
                    "  (        _/  \\___//          `----'   !\n"
                    "  \\    \\       ! \\ \\   \\      /\\ "
                    "   \\___/`------' )       \\            ______/\n"
                    "   \\___/   )  /__/  \\--/   \\ /  \\  "
                    "._    \\      `<         `--_____----'\n"
                    "     \\    /   !       `.    )-   \\/  "
                    ") ___>-_     \\   /-\\    \\    /\n"
                    "     /   !   /         !   !  `.    / /"
                    "      `-_   `-/  /    !   !\n"
                    "    !   /__ /___       /  /__   \\__/ ("
                    "  \\---__/ `-_    /     /  /__\n"
                    "    (______)____)     (______)        \\"
                    "__)         `-_/     (______)\n";
    
    var_set_str_forkey (env, "bear", B);
    *status = 200;
    return 1;
}
