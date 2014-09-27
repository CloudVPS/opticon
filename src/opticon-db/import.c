#include <libopticon/var.h>
#include <libopticon/parse.h>
#include <libopticon/datatypes.h>
#include <libopticon/util.h>
#include <libopticon/import.h>

/** Import a host from json data */
int import_json (host *into, const char *json) {
    var *dat = var_alloc();
    char idstr[32];
    
    if (! parse_json (dat, json)) {
        fprintf (stderr, "Parse error: %s\n", parse_error());
        var_free (dat);
        return 0;
    }
    
    int res = var_to_host (into, dat);
    var_free (dat);
    return res;
}
