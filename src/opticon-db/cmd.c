#include "cmd.h"
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <libopticon/datatypes.h>
#include <libopticon/aes.h>
#include <libopticon/codec_json.h>
#include <libopticon/ioport_file.h>
#include <libopticon/util.h>
#include <libopticonf/var.h>
#include <libopticonf/dump.h>
#include <libopticondb/db_local.h>

#include "import.h"

int cmd_tenant_list (int argc, const char *argv[]) {
    int count = 0;
    char uuidstr[40];
    db *DB = localdb_create (OPTIONS.path);
    uuid *list = db_list_tenants (DB, &count);
    for (int i=0; i<count; ++i) {
        uuid2str (list[i], uuidstr);
        printf ("%s\n", uuidstr);
    }
    db_free (DB);
    free (list);
    return 0;
}

int cmd_tenant_get_metadata (int argc, const char *argv[]) {
    uuid tenant;
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }

    tenant = mkuuid (OPTIONS.tenant);
    db *DB = localdb_create (OPTIONS.path);
    if (! db_open (DB, tenant, NULL)) {
        fprintf (stderr, "%% Could not open %s\n", OPTIONS.tenant);
        db_free (DB);
        return 1;
    }
    var *meta = db_get_metadata (DB);
    dump_var (meta, stdout);
    var_free (meta);
    db_free (DB);
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

int cmd_add_record (int argc, const char *argv[]) {
    uuid tenantid, hostid;
    
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    
    tenantid = mkuuid (OPTIONS.tenant);

    if (OPTIONS.host[0] == 0) {
        fprintf (stderr, "%% No hostid provided\n");
        return 1;
    }
    
    hostid = mkuuid (OPTIONS.host);
    
    if (argc < 3) {
        fprintf (stderr, "%% No filename provided\n");
        return 1;
    }
    
    host *H = host_alloc();
    H->uuid = hostid;
    char *json = load_file (argv[2]);
    if (! json) {
        fprintf (stderr, "%% Could not load %s\n", argv[2]);
        return 1;
    }
    
    if (! import_json (H, json)) {
        free (json);
        host_delete (H);
        return 1;
    }
    
    db *DB = localdb_create (OPTIONS.path);
    if (! db_open (DB, tenantid, NULL)) {
        fprintf (stderr, "%% Could not open database for "
                 "tenant %s\n", OPTIONS.tenant);
        free (json);
        host_delete (H);
        db_free (DB);
        return 1;
    }
    
    if (! db_save_record (DB, time(NULL), H)) {
        fprintf (stderr, "%% Error saving record\n");
        free (json);
        host_delete (H);
        db_free (DB);
        return 1;
    }

    free (json);    
    host_delete (H);
    db_free (DB);
    return 0;
}

int cmd_get_record (int argc, const char *argv[]) {
    uuid tenantid, hostid;
    
    if (OPTIONS.tenant[0] == 0) {
        fprintf (stderr, "%% No tenantid provided\n");
        return 1;
    }
    
    tenantid = mkuuid (OPTIONS.tenant);

    if (OPTIONS.host[0] == 0) {
        fprintf (stderr, "%% No hostid provided\n");
        return 1;
    }
    
    hostid = mkuuid (OPTIONS.host);

    host *H = host_alloc();
    H->uuid = hostid;
    db *DB = localdb_create (OPTIONS.path);
    if (! db_open (DB, tenantid, NULL)) {    
        fprintf (stderr, "%% Could not open database for "
                 "tenant %s\n", OPTIONS.tenant);
        host_delete (H);
        db_free (DB);
        return 1;
    }
    
    if (! db_get_record (DB, OPTIONS.time, H)) {
        fprintf (stderr, "%% Error loading record\n");
        host_delete (H);
        db_free (DB);
        return 1;
    }
    
    ioport *out = ioport_create_filewriter (stdout);
    codec *c = codec_create_json();
    
    codec_encode_host (c, out, H);
    codec_release (c);
    ioport_close (out);
    db_close (DB);
    db_free (DB);
    host_delete (H);
    return 0;
}
