#include <db.h>

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
