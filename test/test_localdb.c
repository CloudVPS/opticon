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

int main (int argc, const char *argv[]) {
    time_t tnow = time (NULL);
    time_t t1 = tnow - 180;
    time_t t2 = tnow - 120;
    time_t t3 = tnow - 60;
    time_t t4 = tnow - 1;
    
    host *h = host_alloc();
    meterid_t M_TEST = makeid ("test",MTYPE_INT,0);
    meter *m_test = host_get_meter (h, M_TEST);
    db *d = db_open_local (".");
    
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
    
    return 0;
}
