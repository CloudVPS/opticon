#include <libopticonf/var.h>
#include "probes.h"

var *runprobe_pcpu (probe *self) {
    var *res = var_alloc();
    var_set_double_forkey (res, "pcpu", ((double)(rand()%1000))/10.0);
    return res;
}

var *runprobe_hostname (probe *self) {
    var *res = var_alloc();
    var_set_str_forkey (res, "hostname", "srv1.heikneuter.nl");
    return res;
}

