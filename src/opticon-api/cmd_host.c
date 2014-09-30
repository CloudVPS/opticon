#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <libopticon/util.h>
#include <libopticon/datatypes.h>
#include <microhttpd.h>
#include "req_context.h"
#include "cmd.h"
#include "options.h"

int cmd_host_get (req_context *ctx, req_arg *a, var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

int cmd_host_list_watchers (req_context *ctx, req_arg *a, 
                            var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

int cmd_host_set_watcher (req_context *ctx, req_arg *a, 
                          var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

int cmd_host_delete_watcher (req_context *ctx, req_arg *a, 
                             var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

int cmd_host_get_range (req_context *ctx, req_arg *a, 
                        var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

int cmd_host_get_time (req_context *ctx, req_arg *a, 
                        var *env, int *status) {
    return err_server_error (ctx, a, env, status);
}

