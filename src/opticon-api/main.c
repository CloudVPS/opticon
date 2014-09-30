#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include "req_context.h"

int authenticate_context (req_context *ctx) {
    return 1;
}

int authorize_context (req_context *ctx, auth_level level) {
    return 1;
}

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
    
    /* Validate the token */
    if (! authenticate_context (ctx)) {
        generate_error_response (connection, 401);
        req_context_free (ctx);
        *con_cls = NULL;
        return MHD_YES;
    }
    
    /* Make sure the token has access to the tenant */
    if (! authorize_context (ctx, AUTH_USER)) {
        generate_error_response (connection, 403);
        req_context_free (ctx);
        *con_cls = NULL;
        return MHD_YES;
    }
    
    /* [/v1.0]/ */
    if (! uuidvalid (ctx->tenantid)) {
        if (! authorize_context (ctx, AUTH_ADMIN)) {
            generate_error_response (connection, 403);
            req_context_free (ctx);
            *con_cls = NULL;
            return MHD_YES;
        }
        
        cmd_list_tenants (ctx, connection);
        req_context_free (ctx);
        *con_cls = NULL;
        return MHD_YES;
    }
    
    /* [/v1.0]/$TENANT[/$CMD[/$ARG]] */
    if (! uuidvalid (ctx->hostid)) {
        /* [/v1.0]/$TENANT */
        if ((! ctx->command) || (! ctx->command[0])) {
            cmd_tenant (ctx, connection);
            req_context_free (ctx);
            *con_cls = NULL;
            return MHD_YES;
        }
        
        /* [/v1.0]/$TENANT/hosts */
        if (strcmp (ctx->command, "hosts") == 0) {
            cmd_hosts_overview (ctx, connection);
            cmd_tenant (ctx, connection);
            req_context_free (ctx);
            *con_cls = NULL;
            return MHD_YES;
        }
        
        /* [/v1.0]/$TENANT/meter[/$METER[/$CMD]] */
        if (strcmp (ctx->command, "meter") == 0) {
            cmd_tenant_meter (ctx, connection);
            req_context_free (ctx);
            *con_cls = NULL;
            return MHD_YES;
        }
        /* [/v1.0]/$TENANT/watcher[/$METER[/$LEVEL]] */
        if (strcmp (ctx->command, "meter") == 0) {
            cmd_tenant_meter (ctx, connection);
            req_context_free (ctx);
            *con_cls = NULL;
            return MHD_YES;
        }
        
        generate_error_respones (connection, 500);
        req_context_free (ctx);
        *con_cls = NULL;
        return MHD_YES;
    }
}
