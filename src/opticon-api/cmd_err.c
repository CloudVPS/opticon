#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <libopticon/util.h>
#include <libopticon/datatypes.h>
#include <microhttpd.h>
#include "req_context.h"
#include "cmd.h"
#include "options.h"

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

int err_bad_request (req_context *ctx, req_arg *a, var *out, int *status) {
    var_set_str_forkey (out, "error", "Bad request");
    *status = 400;
    return 1;    
}

