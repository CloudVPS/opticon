#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libopticondb/db_local.h>
#include <libsvc/cliopt.h>

#include "cmd.h"

optinfo OPTIONS = { "","","" };

int set_tenant (const char *o, const char *v) { 
    OPTIONS.tenant = v;
    return 1;
}

int set_key (const char *o, const char *v) {
    OPTIONS.key = v;
    return 1;
}

int set_path (const char *o, const char *v) {
    OPTIONS.path = v;
    return 1;
}

cliopt CLIOPT[] = {
    {"--tenant","-t",OPT_VALUE,"",set_tenant},
    {"--key","-k",OPT_VALUE,"",set_key},
    {"--path","-p",OPT_VALUE,"/var/opticon/db",set_path},
    {NULL,NULL,0,NULL,NULL}
};

clicmd CLICMD[] = {
    {"tenant-list",cmd_tenant_list},
    {"tenant-create",cmd_tenant_create},
    {"tenant-delete",cmd_tenant_delete},
    {"tenant-get-metadata",cmd_tenant_get_metadata},
    {NULL,NULL}
};

void usage (const char *cmdname) {
    fprintf (stderr,
        "%% Usage: %s <command> [options]\n"
         "  Options:\n"
         "        --path <path to database>\n"
         "\n"
         "  Commands:\n"
         "        tenant-list\n"
         "        tenant-get-metadata --tenant <uuid>\n"
         "        tenant-create --tenant <uuid> [--key <base64>]\n"
         "        tenant-delete --tenant <uuid>\n",
        cmdname);
}

int main (int _argc, const char *_argv[]) {
    int argc = _argc;
    const char **argv = cliopt_dispatch (CLIOPT, _argv, &argc);
    if (! argv) return 1;
    if (argc < 2) {
        usage (argv[0]);
        return 1;
    }
    const char *cmd = argv[1];
    return cliopt_runcommand (CLICMD, cmd, argc, argv);
}
