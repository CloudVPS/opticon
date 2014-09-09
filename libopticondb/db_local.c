#include <db_local.h>
#include <ioport.h>
#include <codec.h>
#include <stdio.h>

#define LOCALDB_OFFS_INVALID 0xffffffffffffffffULL

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
	char *dbpath = (char *) malloc (strlen (ctx->path) + 16);
	if (! dbpath) return NULL;
	sprintf (dbpath, "%s/%u.db", ctx->path, dt);
	return fopen (dbpath, "rw+");
}

/** Open the index file for a specified datestamp */
FILE *localdb_open_indexfile (localdb *ctx, datestamp dt) {
	char *dbpath = (char *) malloc (strlen (ctx->path) + 16);
	if (! dbpath) return NULL;
	sprintf (dbpath, "%s/%u.idx", ctx->path, dt);
	return fopen (dbpath, "rw+");
}

uint64_t localdb_read64 (FILE *fix) {
    uint64_t dt, res;
    fread (&dt, sizeof (res), 1, fix);
    res = ((uint64_t) ntohl (dt & 0xffffffffLLU)) << 32;
    res |= ntohl ((dt & 0xffffffff00000000LLU) >> 32);
    return res;
}

uint64_t localdb_find_index (FILE *fix, time_t ts) {
    uint64_t first_when;
    uint64_t last_when;
    fseek (fix, 0, SEEK_START);
    first_when = localdb_read64 (fix);
    fseek (fix, (2*sizeof(uint_64)), SEEK_END);
    uint64_t count = (ftell (fix) / (2*sizeof(uint64_t)))+1;
    if (count < 2) return 0;
    last_when = localdb_read64 (fix);
    if (first_when > ts) return LOCALDB_OFFS_INVALID;
    if (last_when < ts) return LOCALDB_OFFS_INVALID;
    uint64_t range = last_when - first_when;
    uint64_t diff = ts - first_when;
    uint64_t pos = (count * diff) / range;
    uint64_t tsatpos = localdb_read64 (fix);
    uint64_t lastmatch;
    if (tsatpos <= ts) {
        while (tsatpos <= ts) {
            lastmatch = localdb_read64 (fix);
            tsatpos = localdb_read64 (fix);
        }
        return lastmatch;
    }
    lastmatch = localdb_read64 (fix);
    while (tsatpos > ts) {
        fseek (fix, -(4*sizeof(uint64_t)), SEEK_CUR);
        tsatpos = localdb_read64 (fix);
        lastmatch = localdb_read64 (fix);
    }
    return lastmatch;
}

/** Get record for a specific time stamp. FIXME unimplemented. */
int localdb_get_record (db *d, time_t when, host *into) {
    return 0;
}

/** Get an integer value range for a specific time spam. FIXME unimplemented. */
uint64_t *localdb_get_value_range_int (db *d, time_t start, time_t end,
                                       int numsamples, const char *key,
                                       uint8_t arrayindex) {
    return NULL;
}

/** Get a fractional value range for a specific time spam. FIXME unimplemented.*/
double *localdb_get_value_range_frac (db *d, time_t start, time_t end,
                                      int numsamples, const char *key,
                                      uint8_t arrayindex) {
    return NULL;
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
	
	ioport_write_u64 (ixport, when);
	ioport_write_u64 (ixport, dbpos);
	ioport_close (dbport);
	ioport_close (ixport);
	fclose (dbf);
	fclose (ixf);
	codec_release (cod);
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
