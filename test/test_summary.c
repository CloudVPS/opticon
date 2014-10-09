#include <libopticon/summary.h>
#include <libopticon/host.h>
#include <libopticon/util.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main (int argc, const char *argv[]) {
    summaryinfo inf;
    meterid_t mi_pcpu = makeid ("pcpu",MTYPE_FRAC,0);
    meterid_t mi_status = makeid ("status",MTYPE_STR,0);
    
    summaryinfo_init (&inf);
    summaryinfo_add_summary_avg (&inf, "cpu", mi_pcpu);
    summaryinfo_add_summary_count (&inf, "error", mi_status, "ERROR");
    
    host *h = host_alloc();
    meter *m_pcpu = host_get_meter (h, mi_pcpu);
    meter *m_status = host_get_meter (h, mi_status);
    
    summaryinfo_start_round (&inf);

    meter_setcount (m_pcpu, 0);
    meter_set_frac (m_pcpu, 0, 80.0);
    meter_setcount (m_status, 0);
    meter_set_str (m_status, 0, "OK");
    summaryinfo_add_meterdata (&inf, mi_pcpu, &m_pcpu->d);
    summaryinfo_add_meterdata (&inf, mi_status, &m_status->d);
    
    meter_set_frac (m_pcpu, 0, 60.0);
    summaryinfo_add_meterdata (&inf, mi_pcpu, &m_pcpu->d);
    summaryinfo_add_meterdata (&inf, mi_status, &m_status->d);

    meter_set_frac (m_pcpu, 0, 40.0);
    meter_set_str (m_status, 0, "ERROR");
    summaryinfo_add_meterdata (&inf, mi_pcpu, &m_pcpu->d);
    summaryinfo_add_meterdata (&inf, mi_status, &m_status->d);
    
    meter_set_frac (m_pcpu, 0, 60.0);
    meter_set_str (m_status, 0, "OK");
    summaryinfo_add_meterdata (&inf, mi_pcpu, &m_pcpu->d);
    summaryinfo_add_meterdata (&inf, mi_status, &m_status->d);

    var *res = summaryinfo_tally_round (&inf);
    assert (var_get_int_forkey (res, "error") == 1);
    assert (var_get_double_forkey (res, "cpu") == 60.0);
    return 0;
}
