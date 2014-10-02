#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <libopticon/cliopt.h>
#include <libopticon/util.h>
#include <libopticon/datatypes.h>
#include <libopticon/ioport.h>
#include <libopticon/ioport_buffer.h>
#include <libopticon/parse.h>
#include <libopticon/dump.h>
#include <libopticon/defaultmeters.h>
#include <libopticon/react.h>
#include <libopticon/daemon.h>
#include <libhttp/http.h>
#include <libopticon/log.h>
#include <microhttpd.h>
#include <syslog.h>
#include "req_context.h"
#include "cmd.h"
#include "options.h"
#include "tokencache.h"

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
    
    struct sockaddr *so;
    so = MHD_get_connection_info (connection,
            MHD_CONNECTION_INFO_CLIENT_ADDRESS)->client_addr;
    
    if (so->sa_family == AF_INET) {
        struct sockaddr_in *si = (struct sockaddr_in *) so;        
        inet_ntop (AF_INET, &si->sin_addr, ctx->remote, INET6_ADDRSTRLEN);
    }
    else if (so->sa_family == AF_INET6) {
        struct sockaddr_in6 *si = (struct sockaddr_in6 *) so;
        inet_ntop (AF_INET6, &si->sin6_addr, ctx->remote, INET6_ADDRSTRLEN);
    }
    
    /* Parse post data */
    req_context_parse_body (ctx);
    
    req_matchlist_dispatch (&REQ_MATCHES, url, ctx, connection);
    
    const char *lvl = "AUTH_GUEST";
    if (ctx->userlevel == AUTH_USER) lvl = "AUTH_USER ";
    if (ctx->userlevel == AUTH_ADMIN) lvl = "AUTH_ADMIN";
    
    log_info ("%s [%s] %i %s %s", ctx->remote, lvl, ctx->status, method, url);

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
                          hdr, data, NULL, NULL);

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
        if (strcmp (OPTIONS.adminhost, ctx->remote) != 0) {
            log_error ("Rejected admin authentication from host "
                       "<%s> for reasons of not being <%s>",
                       ctx->remote, OPTIONS.adminhost);

            ctx->userlevel = AUTH_GUEST;
            return err_unauthorized (ctx, a, out, status);
        }
        else if (uuidcmp (ctx->opticon_token, OPTIONS.admintoken)) {
            ctx->userlevel = AUTH_ADMIN;
        }
        else ctx->userlevel = AUTH_USER;
    }
    else if (ctx->openstack_token) {
        tcache_node *cache = tokencache_lookup (ctx->openstack_token);
        if (cache) {
            if (ctx->auth_tenants) {
                free (ctx->auth_tenants);
            }
            ctx->auth_tenants = cache->tenantlist;
            ctx->auth_tenantcount = cache->tenantcount;
            ctx->userlevel = cache->userlevel;
            free (cache);
            if (ctx->userlevel == AUTH_GUEST) {
                return err_unauthorized (ctx, a, out, status);
            }
            return 0;
        }
        if (! handle_openstack_token (ctx)) {
            tokencache_store_invalid (ctx->openstack_token);
            return err_unauthorized (ctx, a, out, status);
        }
        tokencache_store_valid (ctx->openstack_token, ctx->auth_tenants,
                                ctx->auth_tenantcount, ctx->userlevel);
        
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
    
    /* Admin user is ok now */
    if (ctx->userlevel == AUTH_ADMIN) return 0;
    
    /* Other users need to match the tenant */
    if (! ctx->auth_tenantcount) {
        return err_not_allowed (ctx, a, out, status);
    }
    for (int i=0; i<ctx->auth_tenantcount; ++i) {
        if (uuidcmp (ctx->auth_tenants[i], ctx->tenantid)) return 0;
    }
    
    return err_not_allowed (ctx, a, out, status);
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
    _P_ ("/",                         REQ_GET,    cmd_list_tenants);
    _P_ ("/",                         REQ_ANY,    err_method_not_allowed);
    _P_ ("/token",                    REQ_GET,    cmd_token);
    _P_ ("/%U*",                      REQ_ANY,    flt_check_tenant);
    _P_ ("/%U",                       REQ_GET,    cmd_tenant_get);
    _P_ ("/%U",                       REQ_POST,   cmd_tenant_create);
    _P_ ("/%U",                       REQ_PUT,    cmd_tenant_update);
    _P_ ("/%U",                       REQ_DELETE, flt_check_admin);
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

/** Daemon runner. Basically kicks microhttpd in action, then sits
  * back and enjoys the show. */
int daemon_main (int argc, const char *argv[]) {
    if (strcmp (OPTIONS.logpath, "@syslog") == 0) {
        log_open_syslog ("opticon-api", OPTIONS.loglevel);
    }
    else {
        log_open_file (OPTIONS.logpath, OPTIONS.loglevel);
    }

    struct MHD_Daemon *daemon;
    unsigned int flags = MHD_USE_THREAD_PER_CONNECTION;
    daemon = MHD_start_daemon (flags, OPTIONS.port, NULL, NULL,
                               &answer_to_connection, NULL,
                               MHD_OPTION_CONNECTION_LIMIT,
                               (unsigned int) 256,
                               MHD_OPTION_PER_IP_CONNECTION_LIMIT, 
                               (unsigned int) 64,
                               MHD_OPTION_END);
    while (1) sleep (60);
}

/** Set up foreground flag */
int set_foreground (const char *i, const char *v) {
    OPTIONS.foreground = 1;
    return 1;
}

/** Set up configuration file path */
int set_confpath (const char *i, const char *v) {
    OPTIONS.confpath = v;
    return 1;
}

int set_mconfpath (const char *i, const char *v) {
    OPTIONS.mconfpath = v;
    return 1;
}

/** Set up pidfile path */
int set_pidfile (const char *i, const char *v) {
    OPTIONS.pidfile = v;
    return 1;
}

/** Set the logfile path, @syslog for logging to syslog */
int set_logpath (const char *i, const char *v) {
    OPTIONS.logpath = v;
    return 1;
}

/** Handle --loglevel */
int set_loglevel (const char *i, const char *v) {
    if (strcmp (v, "CRIT") == 0) OPTIONS.loglevel = LOG_CRIT;
    else if (strcmp (v, "ERR") == 0) OPTIONS.loglevel = LOG_ERR;
    else if (strcmp (v, "WARNING") == 0) OPTIONS.loglevel = LOG_WARNING;
    else if (strcmp (v, "INFO") == 0) OPTIONS.loglevel = LOG_INFO;
    else if (strcmp (v, "DEBUG") == 0) OPTIONS.loglevel = LOG_DEBUG;
    else OPTIONS.loglevel = LOG_WARNING;
    return 1;
}

/** Handle --admin-token */
int conf_admin_token (const char *id, var *v, updatetype tp) {
    OPTIONS.admintoken = mkuuid (var_get_str (v));
    if (! uuidvalid (OPTIONS.admintoken)) {
        log_error ("Invalid admintoken");
        exit (1);
    }
    return 1;
}

/** Handle auth/admin_host config */
int conf_admin_host (const char *id, var *v, updatetype tp) {
    OPTIONS.adminhost = var_get_str (v);
    return 1;
}

/** Handle auth/keystone_url config */
int conf_keystone_url (const char *id, var *v, updatetype tp) {
    OPTIONS.keystone_url = var_get_str (v);
    return 1;
}

/** Handle network/port config */
int conf_port (const char *id, var *v, updatetype tp) {
    OPTIONS.port = var_get_int (v);
    return 1;
}

/** Handle database/path config */
int conf_dbpath (const char *id, var *v, updatetype tp) {
    OPTIONS.dbpath = var_get_str (v);
    return 1;
}

apioptions OPTIONS;

/** Command line flags */
cliopt CLIOPT[] = {
    {"--foreground","-f",OPT_FLAG,NULL,set_foreground},
    {"--pidfile","-p",OPT_VALUE,
        "/var/run/opticon-api.pid", set_pidfile},
    {"--logfile","-l",OPT_VALUE, "@syslog", set_logpath},
    {"--loglevel","-L",OPT_VALUE, "INFO", set_loglevel},
    {"--config-path","-c",OPT_VALUE,
        "/etc/opticon/opticon-api.conf", set_confpath},
    {"--meter-config-path","-m",OPT_VALUE,
        "/etc/opticon/opticon-meter.conf", set_mconfpath},
    {NULL,NULL,0,NULL,NULL}
};

/** Application main. Handle command line flags, load configurations,
  * initialize, then kick off into daemon mode. */
int main (int _argc, const char *_argv[]) {
    int argc = _argc;
    const char **argv = cliopt_dispatch (CLIOPT, _argv, &argc);
    if (! argv) return 1;

    opticonf_add_reaction ("network/port", conf_port);
    opticonf_add_reaction ("auth/admin_token", conf_admin_token);
    opticonf_add_reaction ("auth/admin_host", conf_admin_host);
    opticonf_add_reaction ("auth/keystone_url", conf_keystone_url);
    opticonf_add_reaction ("database/path", conf_dbpath);
    
    OPTIONS.conf = var_alloc();
    if (! load_json (OPTIONS.conf, OPTIONS.confpath)) {
        log_error ("Error loading %s: %s\n",
                   OPTIONS.confpath, parse_error());
        return 1;
    }

    OPTIONS.mconf = get_default_meterdef();
    if (! load_json (OPTIONS.mconf, OPTIONS.mconfpath)) {
        log_error ("Error loading %s: %s\n",
                   OPTIONS.mconfpath, parse_error());
        return 1;
    }
    
    opticonf_handle_config (OPTIONS.conf);
    tokencache_init();
    setup_matches();

    if (! daemonize (OPTIONS.pidfile, argc, argv, daemon_main,
                     OPTIONS.foreground)) {
        log_error ("Error spawning");
        return 1;
    }
}
