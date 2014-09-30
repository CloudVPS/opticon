#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <libopticon/util.h>
#include <libopticon/datatypes.h>
#include <microhttpd.h>
#include "req_context.h"

req_matchlist REQ_MATCHES;

int enumerate_header (void *cls, enum MHD_ValueKind kind,
                      const char *key, const char *value) {
    req_context *ctx = (req_context *) cls;
    req_context_set_header (ctx, key, value);
    return MHD_YES;
}

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

int cmd_list_tenants (req_context *ctx, req_arg *a, var *out, int *status) {
    var_set_str_forkey (out, "hello", "world");
    *status = 200;
    return 1;    
}

int err_unauthorized (req_context *ctx, req_arg *a, var *out, int *status) {
    var_set_str_forkey (out, "error", "Unauthorized access");
    *status = 401;
    return 1;    
}

int err_not_allowed (req_context *ctx, req_arg *a, var *out, int *status) {
    var_set_str_forkey (out, "error", "Not allowed");
    *status = 403;
    return 1;    
}

int err_not_found (req_context *ctx, req_arg *a, var *out, int *status) {
    var_set_str_forkey (out, "error", "Resource not found");
    *status = 404;
    return 1;
}

int err_method_not_allowed (req_context *ctx, req_arg *a,
                            var *out, int *status) {
    var_set_str_forkey (out, "error", "Method not allowed");
    *status = 405;
    return 1;
}

int flt_check_validuser (req_context *ctx, req_arg *a,
                         var *out, int *status) {
    if (! uuidvalid (ctx->opticon_token)) {
        return err_unauthorized (ctx, a, out, status);
    }
    return 0;
}

int flt_check_admin (req_context *ctx, req_arg *a, var *out, int *status) {
    if (! uuidvalid (ctx->opticon_token)) {
        return err_not_allowed (ctx, a, out, status);
    }
    return 0;
}

int cmd_hoer (req_context *ctx, req_arg *a, var *out, int *status) {
    char buf[1024];
    snprintf (buf, 1023, "%s is a whore", a->argv[0]);
    var_set_str_forkey (out, "truth", buf);
    *status = 200;
    return 1;
}

void setup_matches (void) {
    #define _P_(xx,yy,zz) req_matchlist_add(&REQ_MATCHES,xx,yy,zz)

    _P_ ("*",                       REQ_ANY,    flt_check_validuser);
    _P_ ("/",                       REQ_GET,    flt_check_admin);
    _P_ ("/",                       REQ_GET,    cmd_list_tenants);
    _P_ ("/hoer/%",                 REQ_GET,    cmd_hoer);
/*    _P_ ("/",                       REQ_ANY,    err_method_notimpl);
    _P_ ("/%U",                     REQ_ANY,    flt_check_tenant
    _P_ ("/%U",                     REQ_GET,    cmd_tenant_get);
    _P_ ("/%U",                     REQ_POST,   cmd_tenant_create);
    _P_ ("/%U",                     REQ_PUT,    cmd_tenant_update);
    _P_ ("/%U",                     REQ_DELETE, cmd_tenant_delete);
    _P_ ("/%U/meta",                REQ_GET,    cmd_tenant_get_meta);
    _P_ ("/%U/meta",                REQ_UPDATE, cmd_tenant_set_meta);
    _P_ ("/%U/meta",                REQ_ANY,    err_method_notimpl_;
    _P_ ("/%U/meter",               REQ_GET,    cmd_tenant_list_meters);
    _P_ ("/%U/meter/%s",            REQ_UPDATE, cmd_tenant_set_meter);
    _P_ ("/%U/meter/%s",            REQ_DELETE, cmd_tenant_delete_meter);
    _P_ ("/%U/watcher",             REQ_GET,    cmd_tenant_list_watchers);
    _P_ ("/%U/watcher/%s",          REQ_UPDATE, cmd_tenant_set_watcher);
    _P_ ("/%U/watcher/%s",          REQ_DELETE, cmd_tenant_delete_watcher);
    _P_ ("/%U/host",                REQ_GET,    cmd_tenant_list_hosts);
    _P_ ("/%U/host",                REQ_ANY,    err_method_notimpl);
    _P_ ("/%U/host/%U",             REQ_GET,    cmd_host_get);
    _P_ ("/%U/host/%U",             REQ_ANY,    err_method_notimpl);
    _P_ ("/%U/host/%U/watcher",     REQ_GET,    cmd__host_list_watchers);
    _P_ ("/%U/host/%U/watcher",     REQ_ANY,    err_method_notimpl);
    _P_ ("/%U/host/%U/watcher/%s",  REQ_UPDATE, cmd_host_set_watcher);
    _P_ ("/%U/host/%U/watcher/%s",  REQ_DELETE, cmd_host_delete_watcher);
    _P_ ("/%U/host/%U/range/%T/%T", REQ_GET,    cmd_host_get_range);
    _P_ ("/%U/host/%U/time/%T",     REQ_GET,    cmd_host_get_time);*/
    _P_ ("*",                       REQ_GET,    err_not_found);
    _P_ ("*",                       REQ_ANY,    err_method_not_allowed);
    #undef _P_
}

int main() {
    setup_matches();
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, 8888,
                               NULL, NULL, &answer_to_connection,
                               NULL, MHD_OPTION_END);
    while (1) sleep (60);
}
