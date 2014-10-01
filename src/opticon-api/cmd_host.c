#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <libopticon/util.h>
#include <libopticon/datatypes.h>
#include <libopticon/codec_json.h>
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
    return 0;
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

/** POST /$TENANT/host/$HOST/watcher/$METERID */
int cmd_host_set_watcher (req_context *ctx, req_arg *a, 
                          var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

/** DELETE /$TENANT/host/$HOST/watcher/$METERID */
int cmd_host_delete_watcher (req_context *ctx, req_arg *a, 
                             var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

int cmd_host_get_range (req_context *ctx, req_arg *a, 
                        var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

int cmd_host_get_time (req_context *ctx, req_arg *a, 
                        ioport *outio, int *status) {
    return 0;
}
