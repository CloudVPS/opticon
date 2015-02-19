#ifndef _CMD_H
#define _CMD_H 1

#include <libopticondb/db.h>
#include <libopticondb/db_local.h>
#include <libopticon/var.h>
#include <libopticon/log.h>
#include "req_context.h"

int set_meterid (char *meterid, req_arg *a, int pos);

var *collect_meterdefs (uuid tenant, uuid host, int watchonly);
int err_generic (var *out, const char *fmt, ...);
int err_server_error (req_context *, req_arg *, var *, int *);
int err_unauthorized (req_context *, req_arg *, var *, int *);
int err_not_allowed (req_context *, req_arg *, var *, int *);
int err_not_found (req_context *, req_arg *, var *, int *);
int err_method_not_allowed (req_context *, req_arg *, var *, int *);
int err_bad_request (req_context *, req_arg *, var *, int *);
int err_conflict (req_context *, req_arg *, var *, int *);
int cmd_list_tenants (req_context *, req_arg *, var *, int *);
int cmd_token (req_context *, req_arg *, var *, int *);
int cmd_tenant_get (req_context *, req_arg *, var *, int *);
int cmd_tenant_get_quota (req_context *, req_arg *, var *, int *);
int cmd_tenant_create (req_context *, req_arg *, var *, int *);
int cmd_tenant_update (req_context *, req_arg *, var *, int *);
int cmd_tenant_delete (req_context *, req_arg *, var *, int *);
int cmd_tenant_get_meta (req_context *, req_arg *, var *, int *);
int cmd_tenant_set_meta (req_context *, req_arg *, var *, int *);
int cmd_tenant_get_summary (req_context *, req_arg *, var *, int *);
int cmd_tenant_list_meters (req_context *, req_arg *, var *, int *);
int cmd_tenant_set_meter (req_context *, req_arg *, var *, int *);
int cmd_tenant_delete_meter (req_context *, req_arg *, var *, int *);
int cmd_tenant_list_watchers (req_context *, req_arg *, var *, int *);
int cmd_tenant_set_watcher (req_context *, req_arg *, var *, int *);
int cmd_tenant_delete_watcher (req_context *, req_arg *, var *, int *);
int cmd_tenant_list_hosts (req_context *, req_arg *, var *, int *);
int cmd_host_overview (req_context *, req_arg *, var *, int *);
int cmd_host_get (req_context *, req_arg *, ioport *, int *);
int cmd_host_remove (req_context *, req_arg *, var *, int *);
int cmd_host_list_watchers (req_context *, req_arg *, var *, int *);
int cmd_host_set_watcher (req_context *, req_arg *, var *, int *);
int cmd_host_delete_watcher (req_context *, req_arg *, var *, int *);
int cmd_host_get_range (req_context *, req_arg *, var *, int *);
int cmd_host_get_time (req_context *, req_arg *, ioport *, int *);
int cmd_list_sessions (req_context *, req_arg *, var *, int *);
int cmd_dancing_bears (req_context *, req_arg *, var *, int *);
#endif

