#include <datatypes.h>
#include <util.h>
#include <assert.h>
#include <stdio.h>
#include <auth.h>
#include <codec.h>
#include <compress.h>
#include <aes.h>
#include <db.h>
#include <db_local.h>
#include <util.h>
#include <sys/stat.h>

int main (int argc, const char *argv[]) {
    const char *stenantid = "001b71534f4b4f1cb281cc06b134f98f";
    const char *shostid = "6f943a0d-bcd9-42fa-b0c9-6ede92f9a46a";
    uuid tenantid = mkuuid (stenantid);
    uuid hostid = mkuuid (shostid);

    time_t tnow = time (NULL);
    time_t t1 = tnow - 180;
    time_t t2 = tnow - 120;
    time_t t3 = tnow - 60;
    time_t t4 = tnow - 1;
    
    host *h = host_alloc();
    h->uuid = hostid;
    
    meterid_t M_TEST = makeid ("test",MTYPE_INT,0);
    meter *m_test = host_get_meter (h, M_TEST);
    mkdir ("./tmpdb", 0755);
    db *d = db_open_local ("./tmpdb", tenantid);
    
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
    
    //system ("rm -rf ./tmpdb");
    return 0;
}
