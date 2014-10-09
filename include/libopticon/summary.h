#ifndef _SUMMARY_H
#define _SUMMARY_H 1

#include <libopticon/meter.h>
#include <libopticon/var.h>

typedef enum summarytype_e {
    SUMMARY_NONE,
    SUMMARY_AVG,
    SUMMARY_TOTAL,
    SUMMARY_COUNT
} summarytype;

typedef struct summarydata_s {
    meterid_t    meter;
    fstring      name;
    summarytype  type;
    meterdata    d;
    int          samplecount;
} summarydata;

typedef struct summaryinfo_s {
    int              count;
    summarydata    **array;
} summaryinfo;

summarydata *summarydata_create (void);
void         summarydata_free (summarydata *);
void         summarydata_clear (summarydata *);
void         summarydata_clear_sample (summarydata *);
void         summarydata_add (summarydata *, meterdata *, metertype_t);
void         summaryinfo_init (summaryinfo *);
void         summaryinfo_clear (summaryinfo *);
void         summaryinfo_add_summary_avg (summaryinfo *self, const char *name,
                                          meterid_t m);
void         summaryinfo_add_summary_total (summaryinfo *self, const char *name,
                                            meterid_t m);
void         summaryinfo_add_summary_count (summaryinfo *self, const char *name,
                                            meterid_t m, const char *match);
void         summaryinfo_start_round (summaryinfo *);
void         summaryinfo_add_meterdata (summaryinfo *, meterid_t m, meterdata *d);
var         *summaryinfo_tally_round (summaryinfo *);

/*

#"pcpu" "pcpu" SUMMARY_AVG
#"ok" "state" SUMMARY_COUNT "OK"
#"stale" "state" SUMMARY_COUNT "STALE"
#"warning" "state" SUMMARY_COUNT "WARN"
#"alert" "state" SUMMARY_COUNT "ALERT"
#"critical" "state" SUMMARY_COUNT "CRITICAL"
#"netin" "net/in_kbs" SUMMARY_TOTAL
#"netout" "net/out_kbs" SUMMARY_TOTAL

*/

#endif
