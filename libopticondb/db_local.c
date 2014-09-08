#include <db_local.h>

/** Convert epoch time to a GMT date stamp integer, to be used as part of
  * the filename of underlying database files.
  */
datestamp time2date (time_t in) {
	datestamp res = 0;
	struct tm tmin;
	gmtime_r (&in, &tmin);
	res = 10000 * (1900+tmin.tm_year) +
		  100 * tmin.tm_min + tmin.tm_sec;
	return res;
}

/** Open the database file for a specified datestamp */
FILE *localdb_open_dbfile (localdb *ctx, datestamp dt) {
	char *dbpath = (char *) malloc (strlen (ctx->db.path) + 16);
	if (! dbpath) return NULL;
	sprintf (dbpath, "%s/%u.db", ctx->db.path, dt);
	return fopen (dbpath, "rw+");
}

/** Open the index file for a specified datestamp */
FILE *localdb_open_indexfile (localdb *ctx, datestamp dt) {
	char *dbpath = (char *) malloc (strlen (ctx->db.path) + 16);
	if (! dbpath) return NULL;
	sprintf (dbpath, "%s/%u.idx", ctx->db.path, dt);
	return fopen (dbpath, "rw+");
}

/** Append a record to the database */
int localdb_save_record (db *dbctx, time_t when, host *h) {
	localdb *self = (localdb *) dbctx;
	datestamp dt = time2date (when);
	off_t dbpos = 0;
	
	FILE *dbf = localdb_open_dbfile (self, dt);
	FILE *ixf = localdb_open_indexfile (self, dt);
	ioport *dbport = ioport_create_filewriter (dbf);
	ioport *ixport = ioport_create_filewriter (ixf);
	codec *cod = codec_create_pkt();
	
	fseek (dbf, 0, SEEK_END);
	fseek (ixf, 0, SEEK_END);
	dbpos = ftell (dbf);
	
	ioport_write_encint (dbport, when);
	codec_encode_host (cod, dbport, h);
	
	ioport_write_encint (ixport, when);
	ioport_write_encint (ixport, dbpos);
	ioport_close (dbport);
	ioport_close (ixport);
	fclose (dbf);
	fclose (ixf);
	return 1;
}

/** Open and initialize a localdb handle */
db *db_open_local (const char *path) {
    localdb *res = (localdb *) malloc (sizeof (localdb));
    res->db.get_record = localdb_get_record;
    res->db.get_value_range_int = localdb_get_value_range_int;
    res->db.get_value_range_frac = localdb_get_value_range_frac;
    res->db.save_record = localdb_save_record;
    res->path = strdup (path);
    return (db *) res;
}

