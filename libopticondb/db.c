#include <db.h>

db *db_open_local (const char *path) {
    localdb *res = (localdb *) malloc (sizeof (localdb));
    res->db.get_record = localdb_get_record;
    res->db.get_value_range_int = localdb_get_value_range_int;
    res->db.get_value_range_frac = localdb_get_value_range_frac;
    res->db.save_record = localdb_save_record;
    res->path = strdup (path);
    return (db *) res;
}

int db_get_record (db *d, time_t when, host *into) {
    return d->get_record (d, when, into);
}

uint64_t *db_get_value_range_int (db *d, time_t start, time_t end,
                                  int numsamples, const char *key,
                                  uint8_t index) {
    return d->get_value_range_int (d, start, end, numsamples, key, index);
}

double *db_get_value_range_frac (db *d, time_t start, time_t end,
                                 int numsamples, const char *key,
                                 uint8_t index) {
    return d->get_value_range_frac (d, start, end, numsamples, key, index);
}

int db_save_record (db *d, time_t when, host *what) {
    return d->save_record (d, when, what);
}
