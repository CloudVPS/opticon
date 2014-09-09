#include <db.h>

/** Get a specific record out of the database.
  * \param d The database handle
  * \param when The timestamp (can be up to 90 seconds later than a record's
  *             own time for it to be a valid match).
  * \param into The host to decode the meter data into.
  */
int db_get_record (db *d, time_t when, host *into) {
    return d->get_record (d, when, into);
}

/** Retrieve a timeline for a specific integer value within a set of
  * records in the database.
  * \param d The database handle
  * \param start The start of the timeline
  * \param end The end of the timeline
  * \param numsamples The number of samples to return
  * \param key The key of the value to retrieve out of the records
  * \param index The value's array index
  * \return A pointer to an array of <numsamples> uint64_t values.
  */
uint64_t *db_get_value_range_int (db *d, time_t start, time_t end,
                                  int numsamples, const char *key,
                                  uint8_t index) {
    return d->get_value_range_int (d, start, end, numsamples, key, index);
}

/** Retrieve a timeline for a specific integer value within a set of
  * records in the database.
  * \param d The database handle
  * \param start The start of the timeline
  * \param end The end of the timeline
  * \param numsamples The number of samples to return
  * \param key The key of the value to retrieve out of the records
  * \param index The value's array index
  * \return A pointer to an array of <numsamples> double values.
  */
double *db_get_value_range_frac (db *d, time_t start, time_t end,
                                 int numsamples, const char *key,
                                 uint8_t index) {
    return d->get_value_range_frac (d, start, end, numsamples, key, index);
}

/** Append a host's sample data as a record to the database.
  * \param d The database handle
  * \param when Timestamp to file the record under
  * \param what The host object
  * \return 1 on success, 0 on failure
  */
int db_save_record (db *d, time_t when, host *what) {
    return d->save_record (d, when, what);
}

/** Close a database handle
  * \param d The handle to close
  */
void db_close (db *d) {
    d->close (d);
}
