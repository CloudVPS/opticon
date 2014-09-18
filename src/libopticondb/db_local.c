#include <libopticondb/db_local.h>
#include <libopticon/ioport_file.h>
#include <libopticon/codec.h>
#include <libopticon/util.h>
#include <libopticonf/parse.h>
#include <libopticonf/dump.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

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

/** Bind a localdb to a specific, existing, tenant.
  * \param d The db handle
  * \param tenant The tenant uuid
  * \param options Driver-specific options (unused).
  * \return 1 on success, 0 on failure.
  */
int localdb_open (db *d, uuid tenant, var *options) {
    localdb *self = (localdb *) d;
    char *insertpos = NULL;
    struct stat st;
    char uuidstr[40];
    int prefixlen = strlen (self->pathprefix);
    uuid2str (tenant, uuidstr);
    if (self->path) free (self->path);
    self->path = (char *) malloc (prefixlen + 64);
    strcpy (self->path, self->pathprefix);
    insertpos = self->path + prefixlen;
    if (*(insertpos-1) != '/') {
        insertpos[0] = '/';
        insertpos++;
    }
    sprintf (insertpos, "%c%c", uuidstr[0], uuidstr[1]);
    insertpos += 2;
    if (stat (self->path, &st) != 0) return 0;
    sprintf (insertpos, "/%c%c", uuidstr[2], uuidstr[3]);
    insertpos += 3;
    if (stat (self->path, &st) != 0) return 0;
    sprintf (insertpos, "/%s", uuidstr);
    if (stat (self->path, &st) != 0) return 0;
    strcat (self->path, "/");
    return 1;
}

/** Open the database file for a specified datestamp */
FILE *localdb_open_dbfile (localdb *ctx, uuid hostid, datestamp dt) {
    char uuidstr[40];
    char *dbpath = (char *) malloc (strlen (ctx->path) + 64);
    if (! dbpath) return NULL;

    uuid2str (hostid, uuidstr);
    sprintf (dbpath, "%s/%s/%u.db", ctx->path, uuidstr, dt);
    FILE *res = fopen (dbpath, "a+");
    if (! res) {
        sprintf (dbpath, "%s/%s", ctx->path, uuidstr);
        if (mkdir (dbpath, 0750) != 0) {
            free (dbpath);
            return NULL;
        }
        sprintf (dbpath, "%s/%s/%u.db", ctx->path, uuidstr, dt);
        res = fopen (dbpath, "a+");
    }
    free (dbpath);
    return res;
}

/** Open the index file for a specified datestamp */
FILE *localdb_open_indexfile (localdb *ctx, uuid hostid, datestamp dt) {
    char uuidstr[40];
    char *dbpath = (char *) malloc (strlen (ctx->path) + 64);
    if (! dbpath) return NULL;

    uuid2str (hostid, uuidstr);
    sprintf (dbpath, "%s/%s/%u.idx", ctx->path, uuidstr, dt);
    FILE *res = fopen (dbpath, "a+");
    if (! res) {
        sprintf (dbpath, "%s/%s", ctx->path, uuidstr);
        if (mkdir (dbpath, 0750) != 0) {
            free (dbpath);
            return NULL;
        }
        sprintf (dbpath, "%s/%s/%u.idx", ctx->path, uuidstr, dt);
        res = fopen (dbpath, "a+");
    }
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
    self->path = NULL;
}

/** Implementation for db_free() */
void localdb_free (db *dbctx) {
    localdb *self = (localdb *) dbctx;
    codec_release (self->codec);
}

void recurse_subs (const char *path, int depthleft,
                   int *alloc, int *outsz, uuid **res) {
    char *newpath;
    struct dirent *dir;
    struct stat st;
    DIR *D = opendir (path);
    if (! D) return;
    
    while ((dir = readdir (D))) {
        if (strcmp (dir->d_name, ".") == 0) continue;
        if (strcmp (dir->d_name, "..") == 0) continue;
        newpath = (char *) malloc (strlen(path) + strlen(dir->d_name)+4);
        sprintf (newpath, "%s/%s", path, dir->d_name);
        if (stat (newpath, &st) == 0) {
            if ((st.st_mode & S_IFMT) == S_IFDIR) {
                if (depthleft == 0) {
                    uuid u = mkuuid (dir->d_name);
                    if (u.msb == 0 && u.lsb == 0) continue;
                    if (*outsz == *alloc) {
                        *alloc *= 2;
                        (*res) = (uuid *)
                            realloc (*res, *alloc * sizeof(uuid));
                    }
                    (*res)[*outsz] = u;
                    (*outsz)++;
                }
                else {
                    recurse_subs (newpath, depthleft-1, alloc, outsz, res);
                }
            }
        }
        free (newpath);
    }
}

uuid *localdb_list_tenants (db *d, int *outsz) {
    localdb *self = (localdb *) d;
    int alloc = 4;
    *outsz = 0;
    uuid *res = (uuid *) malloc (4 * sizeof (uuid));
    recurse_subs (self->pathprefix, 2, &alloc, outsz, &res);
    return res;
}


/** Implementation for db_create_tenant(). Creates the
  * necessary directory structures.
  */
int localdb_create_tenant (db *d, uuid tenant, var *meta) {
    localdb *self = (localdb *) d;
    char *insertpos = NULL;
    struct stat st;
    char uuidstr[40];
    int prefixlen = strlen (self->pathprefix);
    uuid2str (tenant, uuidstr);
    char *tmppath = (char *) malloc (prefixlen + 64);
    strcpy (tmppath, self->pathprefix);
    insertpos = tmppath + prefixlen;
    if (*(insertpos-1) != '/') {
        insertpos[0] = '/';
        insertpos++;
    }
    sprintf (insertpos, "%c%c", uuidstr[0], uuidstr[1]);
    insertpos += 2;
    if (stat (tmppath, &st) != 0) {
        if (mkdir (tmppath, 0750) != 0) return 0;
    }
    sprintf (insertpos, "/%c%c", uuidstr[2], uuidstr[3]);
    insertpos += 3;
    if (stat (tmppath, &st) != 0) {
        if (mkdir (tmppath, 0750) != 0) return 0;
    }
    sprintf (insertpos, "/%s", uuidstr);
    if (stat (tmppath, &st) != 0) {
        if (mkdir (tmppath, 0750) != 0) return 0;
    }
    
    if (! meta) {
        free (tmppath);
        return 1;
    }
    
    strcat (tmppath, "/");

    char *metapath = (char *) malloc (strlen (tmppath) + 17);
    char *mtmppath = (char *) malloc (strlen (tmppath) + 24);
    strcpy (metapath, tmppath);
    strcpy (mtmppath, tmppath);
    strcat (metapath, "tenant.metadata");
    strcat (mtmppath, ".tenant.metadata.new");
    FILE *F = fopen (mtmppath, "w");
    if (F) {
        int res = dump_var (meta, F);
        fclose (F);
        if (res) {
            if (rename (mtmppath, metapath) != 0) res = 0;
        }
        if (! res) unlink (mtmppath);
    }
    free (metapath);
    free (mtmppath);
    free (tmppath);
    return 1;
}

/** Utility function for cleaning out disk directories (rm -rf).
  * \param path The directory to remove recursively.
  * \return 1 on success, 0 on failure.
  */
int localdb_remove_dir (const char *path) {
    char *newpath;
    struct dirent *dir;
    struct stat st;
    
    DIR *D = opendir (path);
    if (! D) return 1;
    
    while ((dir = readdir (D))) {
        if (strcmp (dir->d_name, ".") == 0) continue;
        if (strcmp (dir->d_name, "..") == 0) continue;
        newpath = (char *) malloc (strlen(path) + strlen(dir->d_name)+4);
        sprintf (newpath, "%s/%s", path, dir->d_name);
        if (stat (newpath, &st) == 0) {
            if ((st.st_mode & S_IFMT) == S_IFDIR) {
                if (! localdb_remove_dir (newpath)) {
                    closedir (D);
                    free (newpath);
                    return 0;
                }
            }
            else {
                if (unlink (newpath) != 0) {
                    closedir (D);
                    free (newpath);
                    return 0;
                }
            }
        }
        free (newpath);
    }
    
    closedir (D);
    if (rmdir (path) != 0) {
        return 0;
    }
    return 1;
}

/** Implementation for db_remove_tenant() */
int localdb_remove_tenant (db *d, uuid tenantid) {
    localdb *self = (localdb *) d;
    char *insertpos = NULL;
    struct stat st;
    char uuidstr[40];
    int prefixlen = strlen (self->pathprefix);
    uuid2str (tenantid, uuidstr);
    char *tmppath = (char *) malloc (prefixlen + 64);
    strcpy (tmppath, self->pathprefix);
    insertpos = tmppath + prefixlen;
    if (*(insertpos-1) != '/') {
        insertpos[0] = '/';
        insertpos++;
    }
    sprintf (insertpos, "%c%c", uuidstr[0], uuidstr[1]);
    insertpos += 2;
    if (stat (tmppath, &st) != 0) return 1;
    sprintf (insertpos, "/%c%c", uuidstr[2], uuidstr[3]);
    insertpos += 3;
    if (stat (tmppath, &st) != 0) return 1;
    sprintf (insertpos, "/%s", uuidstr);
    if (stat (tmppath, &st) != 0)  return 1;
    if (! localdb_remove_dir (tmppath)) {
        free (tmppath);
        return 0;
    }
    free (tmppath);
    return 1;
}

/** Implementation for db_list_hosts() */
uuid *localdb_list_hosts (db *d, int *outsz) {
    localdb *self = (localdb *) d;
    uuid *res = (uuid *) malloc (4 * sizeof (uuid));
    int alloc = 4;
    *outsz = 0;
    char *tmpstr;
    uuid u;
    
    DIR *D = opendir (self->path);
    struct dirent *dir;
    struct stat st;
    
    if (! D) return res;

    while ((dir = readdir (D))) {
        tmpstr = (char *) malloc (strlen(self->path)+strlen(dir->d_name)+4);
        strcpy (tmpstr, self->path);
        strcat (tmpstr, dir->d_name);
        if (stat (tmpstr, &st) == 0) {
            if ((st.st_mode & S_IFMT) == S_IFDIR) {
                u = mkuuid (dir->d_name);
                if (u.msb || u.lsb) {
                    res[*outsz] = u;
                    *outsz = (*outsz) +1;
                    if (*outsz == alloc) {
                        alloc *= 2;
                        res = (uuid *) realloc (res, alloc*sizeof(uuid));
                    }
                }
            }
        }
        free (tmpstr);
    }
    closedir (D);
    return res;
}

/** Implementation for db_get_metadata() */
var *localdb_get_metadata (db *d) {
    localdb *self = (localdb *) d;
    struct stat st;
    FILE *F;
    
    char *metapath = (char *) malloc (strlen (self->path) + 16);
    strcpy (metapath, self->path);
    strcat (metapath, "tenant.metadata");
    if (stat (metapath, &st) != 0) {
        printf ("Could not stat: %s\n", metapath);
        free (metapath);
        return NULL;
    }
    F = fopen (metapath, "r");
    if (! F) {
        printf ("Could open stat: %s\n", metapath);
        free (metapath);
        return NULL;
    }
    char *data = (char *) malloc (st.st_size + 16);
    fread (data, st.st_size, 1, F);
    data[st.st_size] = 0;
    fclose (F);
    var *res = var_alloc();
    if (! parse_config (res, data)) {
        printf ("Parse error: %s\n", parse_error());
        var_free (res);
        res = NULL;
    }
    free (metapath);
    free (data);
    return res;
}

/** Implementation for db_set_metadata */
int localdb_set_metadata (db *d, var *v) {
    localdb *self = (localdb *) d;
    int res = 0;
    FILE *F;
    char *metapath = (char *) malloc (strlen (self->path) + 17);
    char *tmppath = (char *) malloc (strlen (self->path) + 24);
    strcpy (metapath, self->path);
    strcpy (tmppath, self->path);
    strcat (metapath, "tenant.metadata");
    strcat (tmppath, ".tenant.metadata.new");
    F = fopen (tmppath, "w");
    if (F) {
        res = dump_var (v, F);
        fclose (F);
        if (res) {
            if (rename (tmppath, metapath) != 0) res = 0;
        }
        if (! res) unlink (tmppath);
    }
    free (metapath);
    free (tmppath);
    return res;
}

/** Allocate an unbound localdb object.
  * \param prefix The path prefix for local storage.
  * \return A database handle (or NULL).
  */
db *localdb_create (const char *prefix) {
    localdb *self = (localdb *) malloc (sizeof (localdb));
    if (!self) return NULL;
    
    self->db.open = localdb_open;
    self->db.get_record = localdb_get_record;
    self->db.get_value_range_int = localdb_get_value_range_int;
    self->db.get_value_range_frac = localdb_get_value_range_frac;
    self->db.save_record = localdb_save_record;
    self->db.list_hosts = localdb_list_hosts;
    self->db.get_metadata = localdb_get_metadata;
    self->db.set_metadata = localdb_set_metadata;
    self->db.close = localdb_close;
    self->db.list_tenants = localdb_list_tenants;
    self->db.create_tenant = localdb_create_tenant;
    self->db.remove_tenant = localdb_remove_tenant;
    self->db.free = localdb_free;
    self->db.opened = 0;
    self->db.tenant.lsb = 0;
    self->db.tenant.msb = 0;
    self->pathprefix = strdup (prefix);
    self->path = NULL;
    self->codec = codec_create_pkt();
    return (db *) self;
}
