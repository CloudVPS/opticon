#include "cmd.h"
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <libopticon/datatypes.h>
#include <libopticon/aes.h>
#include <libopticon/util.h>
#include <libopticonf/var.h>
#include <libopticondb/db_local.h>

void recurse_subs (const char *path, int depthleft) {
    char *newpath;
    struct dirent *dir;
    struct stat st;
    DIR *D = opendir (path);
    if (! D) return;
    
    while ((dir = readdir (D))) {
        if (strcmp (dir->d_name, ".") == 0) continue;
        if (strcmp (dir->d_name, "..") == 0) continue;
        newpath = (char *) malloc (strlen(path) + strlen(dir->d_name)+4);
        sprintf (newpath, "%s/%s", path, dir->d_name);
        if (stat (newpath, &st) == 0) {
            if ((st.st_mode & S_IFMT) == S_IFDIR) {
                if (depthleft == 0) {
                    printf ("%s\n", dir->d_name);
                }
                else {
                    recurse_subs (newpath, depthleft-1);
                }
            }
        }
        free (newpath);
    }
}

int cmd_tenant_list (int argc, const char *argv[]) {
    recurse_subs (OPTIONS.path, 2);
    return 0;
}

int cmd_tenant_delete (int argc, const char *argv[]) {
   uuid tenant;
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    
    tenant = mkuuid (OPTIONS.tenant);
    db *DB = localdb_create (OPTIONS.path);
    db_remove_tenant (DB, tenant);
    db_free (DB);
    return 0;
}

int cmd_tenant_create (int argc, const char *argv[]) {
    uuid tenant;
    aeskey key;
    char *strkey;
    
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    
    tenant = mkuuid (OPTIONS.tenant);
    
    if (OPTIONS.key[0] == 0) {
        key = aeskey_create();
    }
    else key = aeskey_from_base64 (OPTIONS.key);
    
    strkey = aeskey_to_base64 (key);
    
    db *DB = localdb_create (OPTIONS.path);
    var *meta = var_alloc();
    var_set_str_forkey (meta, "key", strkey);
    
    if (! db_create_tenant (DB, tenant, meta)) {
        fprintf (stderr, "%% Error creating tenant\n");
        free (strkey);
        var_free (meta);
        db_free (DB);
        return 1;
    }
    
    printf ("Tenant created with key: %s\n", strkey);
    free (strkey);
    var_free (meta);
    db_free (DB);
    return 0;
}
