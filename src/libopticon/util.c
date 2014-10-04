#include <libopticon/datatypes.h>
#include <libopticon/util.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>

/* 5 bit character mapping for ids */
const char *IDTABLE = " abcdefghijklmnopqrstuvwxyz.-_/@";
uint64_t ASCIITABLE[] = {
    // 0x00
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x10
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x20
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28, 27, 30,
    // 0x30
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x40
    31, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    // 0x50
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 0, 0, 0, 0, 29,
    // 0x60
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    // 0x70
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 0, 0, 0, 0, 0
};

/** Construct a meterid.
  * \param label The meter's label (max 11 characters).
  * \param type The meter's type (MTYPE_INT, etc).
  * \return A meterid_t value.
  */
meterid_t makeid (const char *label, metertype_t type, int pos) {
    meterid_t res = (meterid_t) type;
    int bshift = 57;
    const char *crsr = label;
    char c;
    while ((c = *crsr) && bshift > 2) {
        if (c>0) {
            res |= (ASCIITABLE[(int)c] << bshift);
            bshift -= 5;
        }
        crsr++;
    }
    return res;
}

/** Convert an id to a bitmask representing the non-0 caracters */
meterid_t id2mask (meterid_t id) {
    meterid_t res = 0;
    int bshift = 57;
    while (bshift > 2) {
        uint64_t msk = 31ULL << bshift;
        if (! (id & msk)) return res;
        res |= msk;
        bshift -= 5;
    }
    return res;
}

int idisprefix (meterid_t potential, meterid_t prefixfor, meterid_t mask) {
    if ((prefixfor&mask) != (potential&mask)) return 0;
    int bshift = 57;
    while (bshift > 2) {
        uint64_t msk = 31ULL << bshift;
        if (! (mask & msk)) {
            if (((prefixfor & msk) >> bshift) == ASCIITABLE['/']) return 1;
            return 0;
        }
        bshift -= 5;
    }
    return 0;
}

meterid_t idgetprefix (meterid_t id) {
    if (! (id & MMASK_NAME)) return 0ULL;
    meterid_t res = 0ULL;
    int bshift = 57;
    while (bshift > 2) {
        uint64_t msk = 31ULL << bshift;
        uint64_t ch = (id&msk) >> bshift;
        if (IDTABLE[ch] == '/') return res;
        res |= (id & msk);
        bshift -= 5;
    }
    return 0ULL;
}

/** Extract an ASCII name from a meterid_t value.
  * \param id The meterid to parse.
  * \param into String to copy the ASCII data into (minimum size of 12).
  */
void id2str (meterid_t id, char *into) {
    char *out = into;
    char *end = into;
    char t;
    uint64_t tmp;
    int bshift = 57;
    *out = 0;
    while (bshift > 2) {
        tmp = ((id & MMASK_NAME) >> bshift) & 0x1f;
        t = IDTABLE[tmp];
        *out = t;
        if (t != ' ') end = out;
        out++;
        bshift -= 5;
    }
    *(end+1) = 0;
}

/** Extract that 'object' name from a meterid with a path
  * separator in it. Note we only support one level of
  * hierarchy. This is used for json encoding.
  * \param id The meterid, e.g., <<foo/quux>>
  * \param into The string to copy the id into.
  */
void nodeid2str (meterid_t id, char *into) {
    char *out = into;
    char *end = into;
    char t;
    uint64_t tmp;
    int bshift = 57;
    int dowrite = 0;
    *out = 0;
    while (bshift > 2) {
        tmp = ((id & MMASK_NAME) >> bshift) & 0x1f;
        t = IDTABLE[tmp];
        if (t == '/') dowrite = 1;
        else if (dowrite)
        {
            *out = t;
            if (t != ' ') end = out;
            out++;
        }
        bshift -= 5;
    }
    *(end+1) = 0;
}

/** returns the offset of a path separator, if any */
uint64_t idhaspath (meterid_t id) {
    int res = 0;
    char t;
    uint64_t tmp;
    int bshift = 57;
    while (bshift > 2) {
        tmp = ((id & MMASK_NAME) >> bshift) & 0x1f;
        t = IDTABLE[tmp];
        if (t == '/') return idmask(res);
        res++;
        bshift -= 5;
    }
    return 0;
}

/** generate a bitmask for sz characters into a meterid */
uint64_t idmask (int sz) {
    uint64_t mask = 0;
    int bshift = 57;
    for (int i=0; i<sz; ++i) {
        mask |= ((0x1fULL) << bshift);
        bshift -= 5;
    }
    return mask;
}

/** Write out a UUID to a string in ASCII 
  * \param u The UUID
  * \param into String buffer for the result, should fit 36 characters plus
  *             nul-terminator.
  */
void uuid2str (uuid u, char *into) {
    sprintf (into, "%08llx-%04llx-%04llx-"
                   "%04llx-%012llx",
                   ((u.msb & 0xffffffff00000000) >> 32),
                   ((u.msb & 0x00000000ffff0000) >> 16),
                   ((u.msb & 0x000000000000ffff)),
                   ((u.lsb & 0xffff000000000000) >> 48),
                   ((u.lsb & 0x0000ffffffffffff)));
}

char *load_file (const char *fn) {
    struct stat st;
    size_t sz;
    FILE *F;
    char *res;
    
    if (stat (fn, &st) != 0) return NULL;
    res = malloc (st.st_size+1);
    if (! res) return NULL;
    if (! (F = fopen (fn, "r"))) {
        free (res);
        return NULL;
    }
    sz = fread (res, st.st_size, 1, F);
    fclose (F);
    if (sz) {
        res[st.st_size] = 0;
        return res;
    }
    free (res);
    return NULL;
}

void ip2str (struct sockaddr_storage *remote, char *addrbuf) {
    if (remote->ss_family == AF_INET) {
        struct sockaddr_in *in = (struct sockaddr_in *) remote;
        inet_ntop (AF_INET, &in->sin_addr, addrbuf, 64);
    }
    else {
        struct sockaddr_in6 *in = (struct sockaddr_in6 *) remote;
        inet_ntop (AF_INET6, &in->sin6_addr, addrbuf, 64);
    }
}

void str2ip (const char *addrbuf, struct sockaddr_storage *remote) {
    if (strchr (addrbuf, ':')) { /*v6*/
        remote->ss_family = AF_INET6;
        struct sockaddr_in6 *in = (struct sockaddr_in6 *) remote;
        inet_pton (AF_INET6, addrbuf, &in->sin6_addr);
    }
    else {
        remote->ss_family = AF_INET;
        struct sockaddr_in *in = (struct sockaddr_in *) remote;
        inet_pton (AF_INET, addrbuf, &in->sin_addr);
    }
}

static char *tm2str (struct tm *tm, int zulu) {
    /* 1234-67-90T23:56:89Z */
    char *res = (char*) malloc (24);
    sprintf (res, "%i-%02i-%02iT%02i:%02i:%02i%s",
             tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec,
             zulu ? "Z":"");
    return res;
}

char *time2str (time_t ti) {
    struct tm tm;
    localtime_r (&ti, &tm);
    return tm2str (&tm, 0);
}

char *time2utcstr (time_t ti) {
    struct tm tm;
    gmtime_r (&ti, &tm);
    return tm2str (&tm, 1);
}

static void parsetime (const char *str, struct tm *into) {
    if (strlen (str)<18) return;
    into->tm_year = atoi (str) - 1900;
    into->tm_mon = atoi (str+5) - 1;
    into->tm_mday = atoi (str+8);
    into->tm_hour = atoi (str+11);
    into->tm_min = atoi (str+14);
    into->tm_sec = atoi (str+17);
}

time_t str2time (const char *tstr) {
    struct tm tm;
    parsetime (tstr, &tm);
    return mktime (&tm);
}

time_t utcstr2time (const char *tstr) {
    struct tm tm;
    parsetime (tstr, &tm);
    return timegm (&tm);
}
