#include <libopticon/datatypes.h>
#include <libopticon/util.h>
#include <libopticon/auth.h>
#include <libopticon/codec.h>
#include <libopticon/compress.h>
#include <libopticon/aes.h>
#include <libopticondb/db.h>
#include <libopticondb/db_local.h>
#include <libopticon/util.h>
#include <libopticonf/var.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>

int main (int argc, const char *argv[]) {
    const char *stenantid = "001b71534f4b4f1cb281cc06b134f98f";
    const char *shostid = "6f943a0d-bcd9-42fa-b0c9-6ede92f9a46a";
    uuid tenantid = mkuuid (stenantid);
    uuid hostid = mkuuid (shostid);
    var *meta = var_alloc();
    var_set_str_forkey (meta, "key", "dVLihKIvQG1hw6lqYUl4Cg==");

    time_t tnow = time (NULL);
    time_t t1 = tnow - 180;
    time_t t2 = tnow - 120;
    time_t t3 = tnow - 60;
    time_t t4 = tnow - 1;
    
    host *h = host_alloc();
    h->uuid = hostid;
    host_begin_update (h, time (NULL));
    
    meterid_t M_TEST = makeid ("test",MTYPE_INT,0);
    meter *m_test = host_get_meter (h, M_TEST);
    mkdir ("./tmpdb", 0755);
    db *d = localdb_create ("./tmpdb");
    assert (db_create_tenant (d, tenantid, NULL));
    assert (db_open (d, tenantid, NULL));
    assert (db_set_metadata (d, meta));
    var_free (meta);
    
    assert (meta = db_get_metadata (d));
    const char *kstr = var_get_str_forkey (meta, "key");
    assert (strcmp (kstr, "dVLihKIvQG1hw6lqYUl4Cg==") == 0);
    var_free (meta);

    meter_setcount (m_test, 0);
    meter_set_uint (m_test, 0, 10);
    db_save_record (d, t1, h);
    meter_set_uint (m_test, 0, 15);
    db_save_record (d, t2, h);
    meter_set_uint (m_test, 0, 12);
    db_save_record (d, t3, h);
    meter_set_uint (m_test, 0, 24);
    db_save_record (d, t4, h);
    
    assert (db_get_record (d, t1, h));
    assert (meter_get_uint (m_test, 0) == 10);
    assert (db_get_record (d, t2, h));
    assert (meter_get_uint (m_test, 0) == 15);
    assert (db_get_record (d, t3 + 15, h));
    assert (meter_get_uint (m_test, 0) == 12);
    assert (db_get_record (d, tnow, h));
    assert (meter_get_uint (m_test, 0) == 24);
    
    uint64_t *arr = db_get_value_range_int (d, t1, tnow, 4, M_TEST, 0, h);
    assert (arr[0] == 10 && arr[1] == 15 && arr[2] == 12 && arr[3] == 24);
    free (arr);

    uuid *hosts;
    int hostcount;
    assert (hosts = db_list_hosts (d, &hostcount));
    assert (hostcount == 1);
    assert (uuidcmp (hosts[0], hostid));
    free (hosts);
    
    db_close (d);
    assert (db_remove_tenant (d, tenantid));
    assert (! db_open (d, tenantid, NULL));
    system ("rm -rf ./tmpdb");
    db_close (d);
    host_delete (h);
    return 0;
}
