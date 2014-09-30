#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include "req_context.h"

void generate_error_response (struct MHD_Connection *connection,
                              int statuscode, const char *txt) {
    const char *errtxt = txt;
    struct MHD_Response *response;
    char buf[256];
    if (! errtxt) {
        errtxt = "An unkown error occured";
        switch (statuscode) {
            case 401:
                errtxt = "Authentication token needed";
                break;
        
            case 403:
                errtxt = "Insufficient access privileges";
                break;
        
            case 500:
                errtxt = "Internal server error";
                break;
        }
    }
     
    sprintf (buf, "{\"error\":\"%s\"}", errtxt);
    response = MHD_create_response_from_data (strlen (buf), buf);
    MHD_queue_response (connection, statuscode, response);
    MHD_destroy_response (response);       
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
    
    /* Parse post data */
    req_context_parse_body (ctx);
    
    req_matchlist_dispatch (REQ_MATCHES, url, ctx, connection);
    req_context_free (ctx);
    *con_cls = NULL;
    return MHD_YES;
}

void setup_matches (void) {
    #define _P_(xx,yy,zz) req_matchlist_add(REQ_MATCHES,xx,yy,zz)
    #define REQ_UPDATE (REQ_POST|REQ_PUT)
    _P_ ("*",                       REQ_ANY     flt_check_validuser);
    _P_ ("/",                       REQ_GET,    flt_check_admin);
    _P_ ("/",                       REQ_GET,    cmd_list_tenants);
    _P_ ("/",                       REQ_ANY,    err_method_notimpl);
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
    _P_ ("/%U/host/%U/time/%T",     REQ_GET,    cmd_host_get_time);
    _P_ ("*",                       REQ_GET,    err_not_found);
    _P_ ("*",                       REQ_ANY,    err_method_notimpl);
    #undef _P_
}