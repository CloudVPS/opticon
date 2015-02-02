#include <libopticon/log.h>
#include <libopticon/var_dump.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/sysctl.h>

var *runprobe_distro (probe *self) {
    var *res = var_alloc();
    char *distro = NULL;
    char *c;
    FILE *F;
    char buf[256];
    F = popen ("/usr/sbin/system_profiler SPSoftwareDataType","r");
    if (! F) return res;
    while (! feof (F)) {
        *buf = 0;
        fgets (buf, 255, F);
        if (strncmp (buf, "      System Version: ", 22) == 0) {
            distro = buf+22;
            c = strchr (distro, '\n');
            if (c) *c = 0;
            log_debug ("(probe_distro) snarked distro: %s", distro);
            break;
        }
    }
    pclose (F);
    if (distro) {
        var *res_os = var_get_dict_forkey (res, "os");
        var_set_str_forkey (res_os, "distro", distro);
    }
    return res;
}

/** Structure for keeping track of top records */
typedef struct toprec_s {
    uint32_t pid;
    char cmd[32];
    double pcpu;
    uint32_t sizekb;
    char user[32];
} toprec;

/** Structure for keeping all the data for a single top round */
typedef struct topinfo_s {
    toprec records[16];
    double cpu_usage;
    uint32_t memsizekb;
    double load1;
    double load5;
    double load15;
    int numproc;
    int numrun;
    int numstuck;
    uint32_t netin_bytes;
    uint32_t netout_bytes;
    int netin_pkt;
    int netout_pkt;
    int iops_read;
    int iops_write;
    int kmem_avail;
    int kmem_free;
} topinfo;

/** Data that will be collected by the topthread. TOP[1] is the copy that is
  * actively being written to, only after completion will it be copied to TOP[0],
  * which should be considered the canonical copy */
topinfo TOP[2];

typedef enum { TOP_HDR, TOP_BODY } topstate;

/** Utility function for copying a substring of a bit of space-formatted text */
static void cpystr (char *into, const char *ptr, int sz) {
    int eos = 0;
    for (int i=0; i<sz; ++i) {
        if (ptr[i] == '\n') break;
        into[i] = ptr[i];
        if (into[i] != ' ') eos = i+1;
    }
    into[eos] = 0;
}

/** Convert size-information in top output (number + B/M/G/K) to a number in
  * kilobytes.
  */
static uint32_t sz2kb (const char *str) {
    int ival = atoi (str);
    if (! ival) return ival;
    
    const char *c = str;
    while (isdigit (*c)) c++;
    if (*c == 'B') ival /= 1024;
    else if (*c == 'M') ival *= 1024;
    else if (*c == 'G') ival *= (1024*1024);
    return ival;
}

static uint32_t sz2b (const char *str) {
    int ival = atoi (str);
    if (! ival) return ival;
    
    const char *c = str;
    while (isdigit (*c)) c++;
    if (*c == 'K') ival *= 1024;
    else if (*c == 'M') ival *= (1024*1024);
    else if (*c == 'G') ival *= (1024*1024*1024);
    return ival;
}

/** Background thread that spawns /usr/bin/top and lets it run, parsing the output
  * as it comes along. */
void run_top (thread *me) {
    char buf[1024];
    topstate st = TOP_HDR;
    int count = 0;
    int offs_cmd, offs_cpu, offs_mem, offs_user;
    memset (TOP, 0, 2*sizeof(topinfo));
    FILE *f = popen ("/usr/bin/top -l 0 -o cpu -n 16 -c n -s 30", "r");
    if (! f) {
        log_error ("Could not open top probe");
        return;
    }
    while (1) {
        if (feof (f)) {
            log_info ("Reopening top");
            pclose (f);
            f = popen ("/usr/bin/top -l 0 -o cpu -n 16 -c n -s 30", "r");
        }
        
        buf[0] = 0;
        fgets (buf, 1023, f);
        
        if (buf[0] == 'P' && buf[1] == 'I' && buf[2] == 'D') {
            st = TOP_BODY;
            memset (TOP[1].records, 0, 16 *sizeof (toprec));
            offs_cmd = strstr (buf, "COMMAND ") - buf;
            offs_cpu = strstr (buf, "%CPU ") - buf;
            offs_mem = strstr (buf, "MEM ") -buf;
            offs_user = strstr (buf, "USER ") - buf;
            continue;
        }
        if (st == TOP_BODY) {
            if (strlen (buf) < (offs_user+2)) continue;
            if (atof (buf+offs_cpu) < 0.0001) {
                memcpy (TOP, TOP+1, sizeof (topinfo));
                memset (TOP+1, 0, sizeof(topinfo));
                st = TOP_HDR;
                count = 0;
                continue;
            }
            TOP[1].records[count].pid = atoi (buf);
            cpystr (TOP[1].records[count].cmd, buf+offs_cmd, 15);
            TOP[1].records[count].pcpu = atof (buf+offs_cpu);
            TOP[1].records[count].sizekb = sz2kb (buf+offs_mem);
            cpystr (TOP[1].records[count].user, buf+offs_user, 14);
            count++;
            if (count == 14) {
                memcpy (TOP, TOP+1, sizeof (topinfo));
                memset (TOP+1, 0, sizeof(topinfo));
                st = TOP_HDR;
                count = 0;
            }
        }
        else {
            if (memcmp (buf, "Load Avg", 8) == 0) {
                TOP[1].load1 = atof (buf + 10);
                char *next = strchr (buf, ',');
                if (next) {
                    TOP[1].load5 = atof (next+2);
                    next = strchr (next+2, ',');
                    if (next) {
                        TOP[1].load15 = atof (next+2);
                    }
                }
            }
            else if (memcmp (buf, "CPU usage: ", 11) == 0) {
                TOP[1].cpu_usage = atof (buf+11);
                char *next = strchr (buf, ',');
                if (next) TOP[1].cpu_usage += atof (next+2);
            }
            else if (memcmp (buf, "VM: ", 3) == 0) {
                TOP[1].memsizekb = sz2kb (buf+4);
            }
            else if (memcmp (buf, "Processes: ", 11) == 0) {
                TOP[1].numproc = atoi (buf+11);
                char *next = strchr (buf, ',');
                if (! next) continue;
                next++;
                if (*next != ' ') continue;
                next++;
                TOP[1].numrun = atoi (next);
                next = strchr (next, ',');
                if (! next) continue;
                next++;
                if (*next != ' ') continue;
                next++;
                TOP[1].numstuck = atoi (next);
            }
            else if (memcmp (buf, "Networks: packets: ", 19) == 0) {
                TOP[1].netin_pkt = atoi (buf+19);
                char *next = strchr (buf, '/');
                if (! next) continue;
                next++;
                TOP[1].netin_bytes = sz2b (next);
                next = strchr (next, ',');
                if (! next) continue;
                next++;
                if (*next != ' ') continue;
                next++;
                TOP[1].netout_pkt = atoi (next);
                next = strchr (next, '/');
                if (! next) continue;
                next++;
                TOP[1].netout_bytes = sz2b (next);
            }
            else if (memcmp (buf, "Disks: ", 7) == 0) {
                TOP[1].iops_read = atoi (buf+7);
                char *next = strchr (buf, ',');
                if (! next) continue;
                next++;
                if (*next != ' ') continue;
                next++;
                TOP[1].iops_write = atoi (next);
            }
            else if (memcmp (buf, "PhysMem: ", 9) == 0) {
                TOP[1].kmem_avail = sz2kb (buf+9);
                char *next = strchr (buf, '(');
                if (! next) continue;
                next++;
                int kmemused = sz2kb (next);
                next = strchr (next, ',');
                if (! next) continue;
                next++;
                if (*next != ' ') continue;
                next++;
                TOP[1].kmem_avail += sz2kb (next);
                TOP[1].kmem_free = TOP[1].kmem_avail - kmemused;
            }
        }
    }
}

/** The above thread */
thread *TOPTHREAD = NULL;

/** The actual probe. Spawns the thread, reaps the benefits */
var *runprobe_top (probe *self) {
    if (! TOPTHREAD) {
        TOPTHREAD = thread_create (run_top, NULL);
        sleep (3);
    }
    
    topinfo ti;
    memcpy (&ti, TOP, sizeof (topinfo));
    
    var *res = var_alloc();
    var *res_top = var_get_array_forkey (res, "top");
    for (int i=0; ti.records[i].cmd[0] && i<15; ++i) {
        var *procrow = var_add_dict (res_top);
        var_set_int_forkey (procrow, "pid", ti.records[i].pid);
        var_set_str_forkey (procrow, "user", ti.records[i].user);
        var_set_str_forkey (procrow, "name", ti.records[i].cmd);
        var_set_double_forkey (procrow, "pcpu", ti.records[i].pcpu);
        double memperc = (double) ti.records[i].sizekb;
        memperc = memperc / (double) ti.kmem_avail;
        memperc = memperc * 100.0;
        var_set_double_forkey (procrow, "pmem", memperc);
    }
    var_set_double_forkey (res, "pcpu", ti.cpu_usage);
    var *arr = var_get_array_forkey (res, "loadavg");
    var_add_double (arr, ti.load1);
    var_add_double (arr, ti.load5);
    var_add_double (arr, ti.load15);
    var_set_int_forkey (res, "proc/total", ti.numproc);
    var_set_int_forkey (res, "proc/run", ti.numrun);
    var_set_int_forkey (res, "proc/stuck", ti.numstuck);
    var_set_int_forkey (res, "net/in_kbs", ti.netin_bytes/(30*128));
    var_set_int_forkey (res, "net/in_pps", ti.netin_pkt/30);
    var_set_int_forkey (res, "net/out_kbs", ti.netout_bytes/(30*128));
    var_set_int_forkey (res, "net/out_pps", ti.netout_pkt/30);
    var_set_int_forkey (res, "io/rdops", ti.iops_read/30);
    var_set_int_forkey (res, "io/wrops", ti.iops_write/30);
    var_set_int_forkey (res, "mem/total", ti.kmem_avail);
    var_set_int_forkey (res, "mem/free", ti.kmem_free);
    return res;
}

/** Diskfree probe, parses output of /bin/df */
var *runprobe_df (probe *self) {
    char buffer[1024];
    char device[32];
    char mount[32];
    uint64_t szkb;
    double pused;
    char *crsr;
    var *res = var_alloc();
    var *v_df = var_get_array_forkey (res, "df");
    FILE *f = popen ("/bin/df -k", "r");
    while (! feof (f)) {
        buffer[0] = 0;
        fgets (buffer, 1023, f);
        if (memcmp (buffer, "/dev", 4) != 0) continue;
        cpystr (device, buffer, 12);
        crsr = buffer;
        while ((*crsr) && (! isspace (*crsr))) crsr++;
        if (! *crsr) continue;
        while ((*crsr) && (isspace (*crsr))) crsr++;
        if (! *crsr) continue;
        szkb = strtoull (crsr, &crsr, 10);

        /* skip to used */ 
        while ((*crsr) && (! isspace (*crsr))) crsr++;
        if (! *crsr) continue;
        while ((*crsr) && (isspace (*crsr))) crsr++;
        if (! *crsr) continue;
       
        /* skip to available */ 
        while ((*crsr) && (! isspace (*crsr))) crsr++;
        if (! *crsr) continue;
        while ((*crsr) && (isspace (*crsr))) crsr++;
        if (! *crsr) continue;

        /* skip to used% */ 
        while ((*crsr) && (! isspace (*crsr))) crsr++;
        if (! *crsr) continue;
        while ((*crsr) && (isspace (*crsr))) crsr++;
        if (! *crsr) continue;
        
        pused = atof (crsr);

        /* skip to iused */ 
        while ((*crsr) && (! isspace (*crsr))) crsr++;
        if (! *crsr) continue;
        while ((*crsr) && (isspace (*crsr))) crsr++;
        if (! *crsr) continue;

        /* skip to ifree */ 
        while ((*crsr) && (! isspace (*crsr))) crsr++;
        if (! *crsr) continue;
        while ((*crsr) && (isspace (*crsr))) crsr++;
        if (! *crsr) continue;

        /* skip to iused% */ 
        while ((*crsr) && (! isspace (*crsr))) crsr++;
        if (! *crsr) continue;
        while ((*crsr) && (isspace (*crsr))) crsr++;
        if (! *crsr) continue;

        /* skip to iused% */ 
        while ((*crsr) && (! isspace (*crsr))) crsr++;
        if (! *crsr) continue;
        while ((*crsr) && (isspace (*crsr))) crsr++;
        if (! *crsr) continue;
        
        cpystr (mount, crsr, 24);
        var *node = var_add_dict (v_df);
        var_set_str_forkey (node, "device", device);
        var_set_int_forkey (node, "size", (uint32_t) (szkb/1024));
        var_set_double_forkey (node, "pused", pused);
        var_set_str_forkey (node, "mount", mount);
    }
    pclose (f);
    int cnt = var_get_count (v_df);
    f = popen ("/sbin/mount","r");
    while (! feof (f)) {
        buffer[0] = 0;
        fgets (buffer, 1023, f);
        if (memcmp (buffer, "/dev", 4) != 0) continue;
        char *crsr = buffer;
        char *token;
        if ((token = strsep (&crsr, " "))) {
            for (int i=0; i<cnt; ++i) {
                var *node = var_get_dict_atindex (v_df, i);
                const char *devid = var_get_str_forkey (node, "device");
                if (strcmp (devid, token) == 0) {
                    crsr = strchr (crsr, '(');
                    if (! crsr) continue;
                    crsr = crsr+1;
                    if ((token = strsep (&crsr, ","))) {
                        var_set_str_forkey (node, "fs", token);
                        break;
                    }
                }
            }
        }
    }
    pclose (f);
    
    return res;
}

/** Uptime probe. Uses Darwin-specific sysctl. */
var *runprobe_uptime (probe *self) {
    var *res = var_alloc();
    struct timeval boottime;
    size_t len = sizeof (struct timeval);
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };
    if (sysctl (mib, 2, &boottime, &len, NULL, 0) < 0) {
        var_set_int_forkey (res, "uptime", 0);
        return res;
    }
    time_t tnow = time (NULL);
    var_set_int_forkey (res, "uptime", tnow - boottime.tv_sec);
    return res;
}

/** List of built-in probe functions */
builtinfunc BUILTINS[] = {
    GLOBAL_BUILTINS,
    {"probe_top", runprobe_top},
    {"probe_df", runprobe_df},
    {"probe_uptime", runprobe_uptime},
    {"probe_distro", runprobe_distro},
    {NULL, NULL}
};

