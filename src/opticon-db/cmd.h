#ifndef _OPTICONDB_CMD_H
#define _OPTICONDB_CMD_H 1

typedef struct optinfo {
    const char *tenant;
    const char *key;
    const char *path;
} optinfo;

extern optinfo OPTIONS;

int cmd_tenant_list (int argc, const char *argv[]);
int cmd_tenant_get_metadata (int argc, const char *argv[]);
int cmd_tenant_delete (int argc, const char *argv[]);
int cmd_tenant_create (int argc, const char *argv[]);

#endif
