#include <libopticondb/db_local.h>
#include <libopticon/ioport_file.h>
#include <libopticon/codec.h>
#include <libopticon/util.h>
#include <stdio.h>
#include <sys/stat.h>

#define LOCALDB_OFFS_INVALID 0xffffffffffffffffULL

/** Convert epoch time to a GMT date stamp integer, to be used as part of
  * the filename of underlying database files.
  */
datestamp time2date (time_t in) {
    datestamp res = 0;
    struct tm tmin;
    gmtime_r (&in, &tmin);
    res = 10000 * (1900+tmin.tm_year) +
          100 * (1+tmin.tm_mon) + tmin.tm_mday;
    return res;
}

/** Open the database file for a specified datestamp */
FILE *localdb_open_dbfile (localdb *ctx, uuid hostid, datestamp dt) {
    char uuidstr[40];
    char *dbpath = (char *) malloc (strlen (ctx->path) + 64);
    if (! dbpath) return NULL;

    uuid2str (hostid, uuidstr);
    sprintf (dbpath, "%s/%s-%u.db", ctx->path, uuidstr, dt);
    FILE *res = fopen (dbpath, "a+");
    free (dbpath);
    return res;
}

/** Open the index file for a specified datestamp */
FILE *localdb_open_indexfile (localdb *ctx, uuid hostid, datestamp dt) {
    char uuidstr[40];
    char *dbpath = (char *) malloc (strlen (ctx->path) + 64);
    if (! dbpath) return NULL;

    uuid2str (hostid, uuidstr);
    sprintf (dbpath, "%s/%s-%u.idx", ctx->path, uuidstr, dt);
    FILE *res = fopen (dbpath, "a+");
    free (dbpath);
    return res;
}

/** Utility function for reading an encoded 64 bits integer out
  * of a FILE stream. This because setting up an iobuffer for
  * just this task stinks for the index.
  */
uint64_t localdb_read64 (FILE *fix) {
    uint64_t dt, res;
    if (fread (&dt, sizeof (res), 1, fix) == 0) {
        return 0;
    }
    res = ((uint64_t) ntohl (dt & 0xffffffffLLU)) << 32;
    res |= ntohl ((dt & 0xffffffff00000000LLU) >> 32);
    return res;
}

/** Get the offset for the closest matching record to a given timestamp
  * out of an indexfile.
  * \param fix The indexfile
  * \param ts The desired timestamp.
  * \return Offset inside the dbfile of the wanted record, or
  *         LOCALDB_OFFS_INVALID if something went wrong.
  */
uint64_t localdb_find_index (FILE *fix, time_t ts) {
    uint64_t first_when;
    uint64_t last_when;
    if (fseek (fix, 0, SEEK_SET) != 0) {
        return LOCALDB_OFFS_INVALID;
    }
    first_when = localdb_read64 (fix);
    if (fseek (fix, -(2*sizeof(uint64_t)), SEEK_END) != 0) {
        return LOCALDB_OFFS_INVALID;
    }
    uint64_t count = (ftell (fix) / (2*sizeof(uint64_t)))+1;
    if (count < 2) return 0;
    last_when = localdb_read64 (fix);
    if (first_when > ts) return LOCALDB_OFFS_INVALID;
    if (last_when < ts) {
        if (ts-last_when > 90) return LOCALDB_OFFS_INVALID;
        ts = last_when;
    }
    uint64_t range = last_when - first_when;
    uint64_t diff = ts - first_when;
    uint64_t pos = (count * diff) / range;
    if (pos) pos--;
    
    fseek (fix, pos * (2*sizeof(uint64_t)), SEEK_SET);
    
    uint64_t tsatpos = localdb_read64 (fix);
    uint64_t lastmatch;
    uint64_t tmp;
    uint64_t newts;

    if (tsatpos <= ts) {
        while (tsatpos <= ts) {
            tmp = localdb_read64 (fix);
            newts = localdb_read64 (fix);
            if (newts) {
                lastmatch = tmp;
                tsatpos = newts;
            }
            else {
                if (tsatpos == ts) {
                    return tmp;
                }
                return lastmatch;
            }
        }
        return lastmatch;
    }
    lastmatch = localdb_read64 (fix);
    while (tsatpos > ts) {
        if (fseek (fix, -(4*sizeof(uint64_t)), SEEK_CUR) != 0) {
            return lastmatch;
        }
        tsatpos = localdb_read64 (fix);
        lastmatch = localdb_read64 (fix);
    }
    return lastmatch;
}

/** Implementation for db_get_record() */
int localdb_get_record (db *d, time_t when, host *into) {
    localdb *self = (localdb *) d;
    datestamp dt = time2date (when);
    FILE *dbf = localdb_open_dbfile (self, into->uuid, dt);
    FILE *ixf = localdb_open_indexfile (self, into->uuid, dt);
    uint64_t offs = localdb_find_index (ixf, when);
    if (offs == LOCALDB_OFFS_INVALID) {
        fclose (dbf);
        fclose (ixf);
        return 0;
    }
    if (fseek (dbf, offs, SEEK_SET) != 0) {
        fclose (dbf);
        fclose (ixf);
        return 0;
    }
    ioport *dbport = ioport_create_filereader (dbf);
    uint64_t pad = ioport_read_u64 (dbport);
    if (pad != 0) {
        fclose (dbf);
        fclose (ixf);
        ioport_close (dbport);
        return 0;
    }
    (void) ioport_read_u64 (dbport);
    int res = codec_decode_host (self->codec, dbport, into);
    fclose (dbf);
    fclose (ixf);
    ioport_close (dbport);
    return res;
}

void __breakme (void) {}

/** Implementation for db_get_value_range_int() */
uint64_t *localdb_get_value_range_int (db *d, time_t start, time_t end,
                                       int numsamples, meterid_t key,
                                       uint8_t arrayindex, host *h) {
    if (end <= start) return NULL;
    if (numsamples <2) return NULL;
    if (numsamples > 1024) return NULL;
    uint64_t *res = (uint64_t *) malloc ((numsamples+1) * sizeof (uint64_t));
    uint64_t interval = end-start;
    meter *m = host_get_meter (h, key);
    meter_setcount (m, 0);
    int respos = 0;
    time_t pos = start;
    int skip = interval / (numsamples-1);
    while (pos < end) {
        if (! localdb_get_record (d, pos, h)) {
            res[respos++] = 0;
        }
        else {
            res[respos++] = meter_get_uint (m, arrayindex);
        }
        pos += skip;
    }
    localdb_get_record (d, end, h);
    res[respos++] = meter_get_uint (m, arrayindex);
    return res;
}

/** Implementation for db_get_value_range_frac() */
double *localdb_get_value_range_frac (db *d, time_t start, time_t end,
                                      int numsamples, meterid_t key,
                                      uint8_t arrayindex, host *h) {
    if (end <= start) return NULL;
    if (numsamples <2) return NULL;
    if (numsamples > 1024) return NULL;
    double *res = (double *) malloc ((numsamples+1) * sizeof (double));
    uint64_t interval = end-start;
    meter *m = host_get_meter (h, key);
    meter_setcount (m, 0);
    int respos = 0;
    time_t pos = start;
    int skip = interval / (numsamples-1);
    while (pos < end) {
        if (! localdb_get_record (d, pos, h)) {
            res[respos++] = 0.0;
        }
        else {
            res[respos++] = meter_get_frac (m, arrayindex);
        }
        pos += skip;
    }
    localdb_get_record (d, end, h);
    res[respos++] = meter_get_frac (m, arrayindex);
    return res;
}

/** Implementation for db_save_record() */
int localdb_save_record (db *dbctx, time_t when, host *h) {
    localdb *self = (localdb *) dbctx;
    datestamp dt = time2date (when);
    off_t dbpos = 0;
    
    FILE *dbf = localdb_open_dbfile (self, h->uuid, dt);
    FILE *ixf = localdb_open_indexfile (self, h->uuid, dt);
    ioport *dbport = ioport_create_filewriter (dbf);
    ioport *ixport = ioport_create_filewriter (ixf);
    
    fseek (dbf, 0, SEEK_END);
    fseek (ixf, 0, SEEK_END);
    dbpos = ftell (dbf);
    
    ioport_write_u64 (dbport, 0);
    ioport_write_u64 (dbport, when);
    codec_encode_host (self->codec, dbport, h);
    
    ioport_write_u64 (ixport, when);
    ioport_write_u64 (ixport, dbpos);
    ioport_close (dbport);
    ioport_close (ixport);
    fclose (dbf);
    fclose (ixf);
    return 1;
}

/** Implementation for db_close() */
void localdb_close (db *dbctx) {
    localdb *self = (localdb *) dbctx;
    free (self->path);
    codec_release (self->codec);
    free (self);
}

/** Open and initialize a localdb handle */
db *db_open_local (const char *path, uuid tenant) {
    struct stat st;
    char uuidstr[40];
    localdb *res = (localdb *) malloc (sizeof (localdb));
    res->db.get_record = localdb_get_record;
    res->db.get_value_range_int = localdb_get_value_range_int;
    res->db.get_value_range_frac = localdb_get_value_range_frac;
    res->db.save_record = localdb_save_record;
    res->db.close = localdb_close;
    res->path = (char *) malloc (strlen(path) + 96);
    uuid2str (tenant, uuidstr);
    sprintf (res->path, "%s/%c%c", path, uuidstr[0], uuidstr[1]);
    if (stat (res->path, &st) != 0) {
        mkdir (res->path, 0750);
    }
    sprintf (res->path + strlen(res->path), "/%c%c", uuidstr[2], uuidstr[3]);
    if (stat (res->path, &st) != 0) {
        mkdir (res->path, 0750);
    }
    sprintf (res->path + strlen(res->path), "/%s", uuidstr);
    if (stat (res->path, &st) != 0) {
        mkdir (res->path, 0750);
    }
    res->codec = codec_create_pkt();
    return (db *) res;
}
