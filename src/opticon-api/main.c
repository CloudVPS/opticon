#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <libopticon/util.h>
#include <libopticon/datatypes.h>
#include <microhttpd.h>
#include "req_context.h"
#include "cmd.h"
#include "options.h"

req_matchlist REQ_MATCHES;

/** MHD callback function for setting the context headers */
int enumerate_header (void *cls, enum MHD_ValueKind kind,
                      const char *key, const char *value) {
    req_context *ctx = (req_context *) cls;
    req_context_set_header (ctx, key, value);
    return MHD_YES;
}

/** MHD callback function for handling a connection */
int answer_to_connection (void *cls, struct MHD_Connection *connection,
                          const char *url,
                          const char *method, const char *version,
                          const char *upload_data,
                          size_t *upload_data_size,
                          void **con_cls) {
    /* Set up a context if we're at the first callback */
    req_context *ctx = *con_cls;
    if (ctx == NULL) {
        ctx = req_context_alloc();
        req_context_set_url (ctx, url);
        req_context_set_method (ctx, method);
        *con_cls = ctx;
        return MHD_YES;
    }
    
    /* Handle post data */
    if (*upload_data_size) {
        req_context_append_body (ctx, upload_data, *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
    }
    
    MHD_get_connection_values (connection, MHD_HEADER_KIND,
                               enumerate_header, ctx);
    
    /* Parse post data */
    req_context_parse_body (ctx);
    
    req_matchlist_dispatch (&REQ_MATCHES, url, ctx, connection);
    req_context_free (ctx);
    *con_cls = NULL;
    return MHD_YES;
}

/** Filter that croaks when no valid token was set */
int flt_check_validuser (req_context *ctx, req_arg *a,
                         var *out, int *status) {
    if (! uuidvalid (ctx->opticon_token)) {
        return err_unauthorized (ctx, a, out, status);
    }
    ctx->userlevel = AUTH_ADMIN;
    return 0;
}

/** Filter that croaks when no valid token was set, or the user behind
  * said token is a filthy peasant.
  */
int flt_check_admin (req_context *ctx, req_arg *a, var *out, int *status) {
    if (! uuidvalid (ctx->opticon_token)) {
        return err_not_allowed (ctx, a, out, status);
    }
    if (! ctx->userlevel != AUTH_ADMIN) {
        return err_not_allowed (ctx, a, out, status);
    }
    return 0;
}

/** Filter that extracts the tenantid argumnt from the url, and croaks
  * when it is invalid. FIXME: Should also croak when the user has
  * no access to the tenant, on account of being a filthy peasant with
  * the wrong UUID.
  */
int flt_check_tenant (req_context *ctx, req_arg *a, var *out, int *status) {
    if (a->argc<1) return err_server_error (ctx, a, out, status);
    ctx->tenantid = mkuuid (a->argv[0]);
    if (! uuidvalid (ctx->tenantid)) { 
        return err_server_error (ctx, a, out, status);
    }
    return 0;
}

/** Filter that extracts the host uuid argument from a url. */
int flt_check_host (req_context *ctx, req_arg *a, var *out, int *status) {
    if (a->argc<2) return err_server_error (ctx, a, out, status);
    ctx->hostid = mkuuid (a->argv[1]);
    if (! uuidvalid (ctx->hostid)) { 
        return err_server_error (ctx, a, out, status);
    }
    return 0;
}

/** Set up all the url routing */
void setup_matches (void) {
    #define _P_(xx,yy,zz) req_matchlist_add(&REQ_MATCHES,xx,yy,zz)

    _P_ ("*",                         REQ_ANY,    flt_check_validuser);
    _P_ ("/",                         REQ_GET,    flt_check_admin);
    _P_ ("/",                         REQ_GET,    cmd_list_tenants);
    _P_ ("/",                         REQ_ANY,    err_method_not_allowed);
    _P_ ("/%U*",                      REQ_ANY,    flt_check_tenant);
    _P_ ("/%U",                       REQ_GET,    cmd_tenant_get);
    _P_ ("/%U",                       REQ_POST,   cmd_tenant_create);
    _P_ ("/%U",                       REQ_PUT,    cmd_tenant_update);
    _P_ ("/%U",                       REQ_DELETE, cmd_tenant_delete);
    _P_ ("/%U/meta",                  REQ_GET,    cmd_tenant_get_meta);
    _P_ ("/%U/meta",                  REQ_UPDATE, cmd_tenant_set_meta);
    _P_ ("/%U/meta",                  REQ_ANY,    err_method_not_allowed);
    _P_ ("/%U/meter",                 REQ_GET,    cmd_tenant_list_meters);
    _P_ ("/%U/meter/%s",              REQ_UPDATE, cmd_tenant_set_meter);
    _P_ ("/%U/meter/%s/%s",           REQ_UPDATE, cmd_tenant_set_meter);
    _P_ ("/%U/meter/%s",              REQ_DELETE, cmd_tenant_delete_meter);
    _P_ ("/%U/meter/%s/%s",           REQ_DELETE, cmd_tenant_delete_meter);
    _P_ ("/%U/watcher",               REQ_GET,    cmd_tenant_list_watchers);
    _P_ ("/%U/watcher/%s",            REQ_UPDATE, cmd_tenant_set_watcher);
    _P_ ("/%U/watcher/%s/%s",         REQ_UPDATE, cmd_tenant_set_watcher);
    _P_ ("/%U/watcher/%s",            REQ_DELETE, cmd_tenant_delete_watcher);
    _P_ ("/%U/watcher/%s/%s",         REQ_DELETE, cmd_tenant_delete_watcher);
    _P_ ("/%U/host",                  REQ_GET,    cmd_tenant_list_hosts);
    _P_ ("/%U/host",                  REQ_ANY,    err_method_not_allowed);
    _P_ ("/%U/host/%U*",              REQ_ANY,    flt_check_host);
    _P_ ("/%U/host/%U",               REQ_GET,    cmd_host_get);
    _P_ ("/%U/host/%U",               REQ_ANY,    err_method_not_allowed);
    _P_ ("/%U/host/%U/watcher",       REQ_GET,    cmd_host_list_watchers);
    _P_ ("/%U/host/%U/watcher",       REQ_ANY,    err_method_not_allowed);
    _P_ ("/%U/host/%U/watcher/%s",    REQ_UPDATE, cmd_host_set_watcher);
    _P_ ("/%U/host/%U/watcher/%s/%s", REQ_UPDATE, cmd_host_set_watcher);
    _P_ ("/%U/host/%U/watcher/%s",    REQ_DELETE, cmd_host_delete_watcher);
    _P_ ("/%U/host/%U/watcher/%s/%s", REQ_DELETE, cmd_host_delete_watcher);
    _P_ ("/%U/host/%U/range/%T/%T",   REQ_GET,    cmd_host_get_range);
    _P_ ("/%U/host/%U/time/%T",       REQ_GET,    cmd_host_get_time);
    _P_ ("*",                         REQ_GET,    err_not_found);
    _P_ ("*",                         REQ_ANY,    err_method_not_allowed);
    #undef _P_
}

apioptions OPTIONS;

int main() {
    OPTIONS.dbpath = "/Users/pi/var";
    OPTIONS.port = 8888;
    setup_matches();
    struct MHD_Daemon *daemon;
    unsigned int flags = MHD_USE_THREAD_PER_CONNECTION |
                         MHD_USE_IPv6;
    daemon = MHD_start_daemon (flags, OPTIONS.port, NULL, NULL,
                               &answer_to_connection, NULL,
                               MHD_OPTION_CONNECTION_LIMIT,
                               (unsigned int) 256,
                               MHD_OPTION_PER_IP_CONNECTION_LIMIT, 
                               (unsigned int) 64,
                               MHD_OPTION_END);
    while (1) sleep (60);
}
