#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <libopticon/util.h>
#include <libopticon/datatypes.h>
#include <libopticon/ioport.h>
#include <libopticon/ioport_buffer.h>
#include <libopticon/parse.h>
#include <libopticon/dump.h>
#include <microhttpd.h>
#include <curl/curl.h>
#include "req_context.h"
#include "cmd.h"
#include "options.h"

req_matchlist REQ_MATCHES;

size_t curlwrite (char *ptr, size_t sz, size_t n, void *pport) {
    ioport *io = (ioport *) pport;
    size_t outsz = sz*n;
    if (ioport_write (io, ptr, outsz)) return outsz;
    return 0;
}

size_t curlread (void *ptr, size_t sz, size_t nm, void *pport) {
    ioport *io = (ioport *) pport;
    size_t insz = sz*nm;
    size_t avail = ioport_read_available (io);
    if (avail < insz) insz = avail;
    if (! insz) return insz;
    if (ioport_read (io, ptr, insz)) return insz;
    return 0;
}

var *http_call (const char *mth, const char *url, var *hdr, var *data) {
    CURL *curl;
    CURLcode curlres;
    struct curl_slist *chunk = NULL;
    
    curl = curl_easy_init();
    if (! curl) return NULL;
    
    ioport *indata = ioport_create_buffer (NULL, 4096);
    ioport *outdata = ioport_create_buffer (NULL, 4096);
    
    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, curlwrite);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, indata);
    
    if (strcmp (mth, "POST") == 0) {
        curl_easy_setopt (curl, CURLOPT_POST, 1L);
    }
    else if (strcmp (mth, "PUT") == 0) {
        curl_easy_setopt (curl, CURLOPT_PUT, 1L);
    }
    else if (strcmp (mth, "DELETE") == 0) {
        curl_easy_setopt (curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    
    if (var_get_count (data) > 0) {
        write_var (data, outdata);
        curl_easy_setopt (curl, CURLOPT_READFUNCTION, curlread);
        curl_easy_setopt (curl, CURLOPT_READDATA, outdata);
        var_set_str_forkey (hdr, "Content-Type", "application/json");
    }
    
    if (var_get_count (hdr) > 0) {
        char tmphdr[1024];
        
        var *myhdr = hdr->value.arr.first;
        while (myhdr) {
            snprintf (tmphdr, 1023, "%s: %s", myhdr->id, var_get_str(myhdr));
            tmphdr[1023] = 0;
            chunk = curl_slist_append (chunk, tmphdr);
            myhdr = myhdr->next;
        }
        
        if (chunk) {
            curl_easy_setopt (curl, CURLOPT_HTTPHEADER, chunk);
        }
    }
    
    var *res = var_alloc();
    
    curlres = curl_easy_perform (curl);
    
    if (curlres == CURLE_OK) {
        parse_json (res, ioport_get_buffer (indata));
    }
    else {
        printf ("%s\n", curl_easy_strerror (curlres));
    }
    
    ioport_close (indata);
    ioport_close (outdata);        
    curl_easy_cleanup (curl);
    if (chunk) curl_slist_free_all (chunk);
    return res;    
}

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

int handle_openstack_token (req_context *ctx) {
    int retcode = 0;
    int i = 0;
    var *hdr = var_alloc();
    var_set_str_forkey (hdr, "X-Auth-Token", ctx->openstack_token);
    var *data = var_alloc();
    var *res = http_call ("GET",
                          "https://identity.stack.cloudvps.com/v2.0/tenants",
                          hdr, data);

    if (ctx->auth_tenants) {
        free (ctx->auth_tenants);
        ctx->auth_tenants = NULL;
    }

    if (res) {
        var *res_tenants = var_get_array_forkey (res, "tenants");
        ctx->auth_tenantcount = var_get_count (res_tenants);
        if (ctx->auth_tenantcount) {
            ctx->auth_tenants = (uuid *) malloc (ctx->auth_tenantcount * sizeof (uuid));
            i = 0;
            var *crsr = res_tenants->value.arr.first;
            while (crsr) {
                ctx->auth_tenants[i++] = var_get_uuid_forkey (crsr, "id");
                const char *strid = var_get_str_forkey (crsr, "id");
                const char *tname = var_get_str_forkey (crsr, "name");
                if (strid && tname) {
                    var_set_str_forkey (ctx->auth_data, strid, tname);
                }
                crsr = crsr->next;
            }
            if (i) {
                retcode = 1;
                ctx->userlevel = AUTH_USER;
            }
        }
        var_free (res);
    }
    var_free (hdr);
    var_free (data);
    return retcode;;
}

/** Filter that croaks when no valid token was set */
int flt_check_validuser (req_context *ctx, req_arg *a,
                         var *out, int *status) {
    if (uuidvalid (ctx->opticon_token)) {
        if (uuidcmp (ctx->opticon_token, OPTIONS.admintoken)) {
            ctx->userlevel = AUTH_ADMIN;
        }
        else ctx->userlevel = AUTH_USER;
    }
    else if (ctx->openstack_token) {
        if (! handle_openstack_token (ctx)) {
            return err_unauthorized (ctx, a, out, status);
        }
    }
    else { 
        return err_unauthorized (ctx, a, out, status);
    }
    return 0;
}

/** Filter that croaks when no valid token was set, or the user behind
  * said token is a filthy peasant.
  */
int flt_check_admin (req_context *ctx, req_arg *a, var *out, int *status) {
    if (! uuidvalid (ctx->opticon_token)) {
        return err_not_allowed (ctx, a, out, status);
    }
    if (ctx->userlevel != AUTH_ADMIN) {
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
    #define _T_(xx,yy,zz) req_matchlist_add_text(&REQ_MATCHES,xx,yy,zz)

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
    _T_ ("/%U/host/%U",               REQ_GET,    cmd_host_get);
    _P_ ("/%U/host/%U",               REQ_ANY,    err_method_not_allowed);
    _P_ ("/%U/host/%U/watcher",       REQ_GET,    cmd_host_list_watchers);
    _P_ ("/%U/host/%U/watcher",       REQ_ANY,    err_method_not_allowed);
    _P_ ("/%U/host/%U/watcher/%s",    REQ_UPDATE, cmd_host_set_watcher);
    _P_ ("/%U/host/%U/watcher/%s/%s", REQ_UPDATE, cmd_host_set_watcher);
    _P_ ("/%U/host/%U/watcher/%s",    REQ_DELETE, cmd_host_delete_watcher);
    _P_ ("/%U/host/%U/watcher/%s/%s", REQ_DELETE, cmd_host_delete_watcher);
    _P_ ("/%U/host/%U/range/%T/%T",   REQ_GET,    cmd_host_get_range);
    _T_ ("/%U/host/%U/time/%T",       REQ_GET,    cmd_host_get_time);
    _P_ ("*",                         REQ_GET,    err_not_found);
    _P_ ("*",                         REQ_ANY,    err_method_not_allowed);
    #undef _P_
    #undef _T_
}

apioptions OPTIONS;

int main() {
    OPTIONS.dbpath = "/Users/pi/var";
    OPTIONS.port = 8888;
    OPTIONS.admintoken = mkuuid ("a1bc7681-b616-4805-90c2-f9a08ce460d3");
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
