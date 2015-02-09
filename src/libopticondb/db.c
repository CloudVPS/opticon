#include <libopticondb/db.h>

/** Open the database for a specific tenant.
  * \param d The database handle
  * \param tenant The tenant to open.
  * \param extra Optional connection information (or NULL).
  * \return 1 on success, 0 on failure.
  */
int db_open (db *d, uuid tenant, var *extra) {
    if (d->open (d, tenant, extra)) {
        d->opened = 1;
        return 1;
    }
    d->opened = 0;
    return 0;
}

/** Get a specific record out of the database.
  * \param d The database handle
  * \param when The timestamp (can be up to 90 seconds later than a record's
  *             own time for it to be a valid match).
  * \param into The host to decode the meter data into.
  */
int db_get_record (db *d, time_t when, host *into) {
    if (! d->opened) return 0;
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
                                  int numsamples, meterid_t key,
                                  uint8_t index, host *h) {
    if (! d->opened) return NULL;
    return d->get_value_range_int (d, start, end, numsamples, key, index, h);
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
                                 int numsamples, meterid_t key,
                                 uint8_t index, host *h) {
    if (! d->opened) return NULL;
    return d->get_value_range_frac (d, start, end, numsamples, key, index, h);
}

/** Append a host's sample data as a record to the database.
  * \param d The database handle
  * \param when Timestamp to file the record under
  * \param what The host object
  * \return 1 on success, 0 on failure
  */
int db_save_record (db *d, time_t when, host *what) {
    if (! d->opened) return 0;
    return d->save_record (d, when, what);
}

/** Delete all data for a host on a specific date.
  * \param d The database handle
  * \param hostid The host uuid
  * \param dt The date to expunge.
  */
void db_delete_host_date (db *d, uuid hostid, datestamp dt) {
    if (! d->opened) return;
    d->delete_host_date (d, hostid, dt);
}

/** Get the metadata associated with a host.
  * \param d The database handle
  * \param hostid The host uuid
  * \return A var object, or NULL on failure.
  */
var *db_get_hostmeta (db *d, uuid hostid) {
    if (! d->opened) return NULL;
    return d->get_hostmeta (d, hostid);
}

/** Get the last time the host metadata was changed.
  * \param d The database handle
  * \pram hostid The host uuid
  * \return A unix timestamp (or 0 on failure).
  */
time_t db_get_hostmeta_changed (db *d, uuid hostid) {
    if (! d->opened) return 0;
    return d->get_hostmeta_changed (d, hostid);
}

/** Set the metadata associated with a host.
  * \param d The database handle
  * \param hostid The host uuid
  * \param v The new metadata
  * \return 1 on success, 0 on failure.
  */
int db_set_hostmeta (db *d, uuid hostid, var *v) {
    if (! d->opened) return 0;
    return d->set_hostmeta (d, hostid, v);
}

/** Get usage information about a specific host */
int db_get_usage (db *d, usage_info *into, uuid hostid) {
    return d->get_usage (d, into, hostid);
}

/** List all hosts stored for the bound tenant.
  * \param d The database handle.
  * \param outsz Pointer to int that will contain the count.
  * \return Array of uuids (caller should free() memory).
  */
uuid *db_list_hosts (db *d, int *outsz) {
    if (! d->opened) return 0;
    return d->list_hosts (d, outsz);
}

/** Retrieve the metadata for the bound tenant of a database.
  * \param d The database (should be bound with db_open()).
  * \return The metadata, or NULL if none could be loaded.
  */
var *db_get_metadata (db *d) {
    if (! d->opened) return NULL;
    return d->get_metadata (d);
}

/** Update the metadata for the bound tenant of a database.
  * \param d The database (should be bound with db_open()).
  * \param v The new metadata.
  * \return 1 on success, 0 on failure.
  */
int db_set_metadata (db *d, var *v) {
    if (! d->opened) return 0;
    return d->set_metadata (d, v);
}

/** Remove host for bound tenant.
  * \param d The bound database handle.
  * \param u The host uuid.
  * \return 1 on success, 0 on failure.
  */
int db_remove_host (db *d, uuid u) {
    return d->remove_host (d, u);
}

/** Retrieve tenant summary information */
var *db_get_summary (db *d) {
    if (! d->opened) return NULL;
    return d->get_summary (d);
}

/** Set tenant summary information */
int db_set_summary (db *d, var *v) {
    if (! d->opened) return 0;
    return d->set_summary (d, v);
}

/** Get tenant host overview information */
var *db_get_overview (db *d) {
    if (! d->opened) return NULL;
    return d->get_overview (d);
}

/** Write tenant host overview */
int db_set_overview (db *d, var *v) {
    if (! d->opened) return 0;
    return d->set_overview (d, v);
}

/** Close a database handle
  * \param d The handle to close
  */
void db_close (db *d) {
    d->close (d);
    d->opened = 0;
}

/** Set global metadata
  * \param d The database handle
  * \param id The id of the metadata to save
  * \param v The metadata
  */
int db_set_global (db *d, const char *id, var *v) {
    return d->set_global (d, id, v);
}

/** Get global metadata
  * \param d The database handle
  * \param id The id of the metadata to load
  * \return Loaded metadata, or NULL.
  */
var *db_get_global (db *d, const char *id) {
    return d->get_global (d, id);
}

/** Free up all resources associated with a db handle */
void db_free (db *d) {
    if (d->opened) d->close (d);
    d->free (d);
    free (d);
}

/** Get a list of all tenants in the database.
  * \param d The database handle.
  * \param cnt Output parameter: result count
  * \return Array of uuids, caller should free() after use.
  */
uuid *db_list_tenants (db *d, int *cnt) {
    return d->list_tenants (d, cnt);
}

/** Create a new tenant.
  * \param d The database handle.
  * \param u The tenant's uuid.
  * \param meta Initial tenant metadata (NULL for none).
  * \return 1 on success, 0 on failure.
  */
int db_create_tenant (db *d, uuid u, var *meta) {
    return d->create_tenant (d, u, meta);
}

/** Remove a tenant and all its hosts.
  * \param d The database handle.
  * \param u The tenant uuid.
  * \return 1 on success, 0 on failure.
  */
int db_remove_tenant (db *d, uuid u) {
    return d->remove_tenant (d, u);
}

