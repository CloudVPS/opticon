#include <db_local.h>

datestamp time2date (time_t in) {
	datestamp res = 0;
	struct tm tmin;
	gmtime_r (&in, &tmin);
	res = 10000 * (1900+tmin.tm_year) +
		  100 * tmin.tm_min + tmin.tm_sec;
	return res;
}

FILE *localdb_open_dbfile (localdb *ctx, datestamp dt) {
	char *dbpath = (char *) malloc (strlen (ctx->db.path) + 16);
	if (! dbpath) return NULL;
	sprintf (dbpath, "%s/%u.db", ctx->db.path, dt);
	return fopen (dbpath, "rw+");
}

FILE *localdb_open_indexfile (localdb *ctx, datestamp dt) {
	char *dbpath = (char *) malloc (strlen (ctx->db.path) + 16);
	if (! dbpath) return NULL;
	sprintf (dbpath, "%s/%u.idx", ctx->db.path, dt);
	return fopen (dbpath, "rw+");
}

db *db_open_local (const char *path) {
    localdb *res = (localdb *) malloc (sizeof (localdb));
    res->db.get_record = localdb_get_record;
    res->db.get_value_range_int = localdb_get_value_range_int;
    res->db.get_value_range_frac = localdb_get_value_range_frac;
    res->db.save_record = localdb_save_record;
    res->path = strdup (path);
    return (db *) res;
}

