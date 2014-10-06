#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include "tproc.h"
#include "wordlist.h"

static int KMEMTOTAL = 1024;

static struct linuxprobe_info_s {
    time_t      lastrun;
    uint64_t    total_cpu;
    uint64_t    net_in;
    uint64_t    net_out;
    uint64_t    io_blk_r;
    uint64_t    io_blk_w;
    uint64_t    io_wait;
    uint16_t    ports[65536][3];
} GLOB;

var *runprobe_uptime (probe *self) {
    char buf[256];
    var *res = var_alloc();
    FILE *F = fopen ("/proc/uptime","r");
    if (F) {
        fgets (buf, 255, F);
        fclose (F);
        var_set_int_forkey ("uptime", strtoull (buf, NULL, 10));
    }
    return res;
}

var *runprobe_io (probe *self)
{
	wordlist *split;
	FILE *F;
	time_t ti;
	char buf[256];
	uint64_t totalblk_r = 0;
	uint64_t totalblk_w = 0;
	uint64_t delta_r;
	uint64_t delta_w;
	uint64_t delta;
	uint64_t cpudelta;
	uint64_t totalcpu;
	uint64_t totalwait = 0;
	var *res = var_alloc();
	
	F = fopen ("/proc/diskstats", "r");
	if (F)
	{
		while (! feof (F))
		{
			buf[0] = 0;
			fgets (buf, 255, F);
			if (! strlen (buf)) continue;
			
			split = wordlist_make (buf);
			if (split->argc < 14)
			{
				wordlist_free (split);
				continue;
			}
			
			totalblk_r += atoll (split->argv[5]);
			totalblk_w += atoll (split->argv[9]);
			
			wordlist_free (split);
		}
		
		fclose (F);
	}

	F = fopen ("/proc/stat","r");
	if (F)
	{
		fgets (buf, 255, F);
		split = wordlist_make (buf);
		totalwait = atoll (split->argv[5]);
		totalcpu = atoll (split->argv[1]) + atoll (split->argv[2]) +
				   atoll (split->argv[3]) + atoll (split->argv[4]);
		wordlist_free (split);
		fclose (F);
	}
	
	delta_r = totalblk_r - GLOB.io_blk_r;
	delta_w = totalblk_w - GLOB.io_blk_w;
	
	ti = time (NULL);
	if (ti == GLOB.lastrun) GLOB.lastrun--;
	if (GLOB.io_blk_r || GLOB.io_blk_w)
	{
	    var *res_io = var_get_dict_forkey (res, "io");
	    if (GLOB.io_blk_r) {
    	    var_set_int_forkey (res_io, "rdops", delta_r / (ti - GLOB.lastrun));
    	}
    	if (GLOB.io_blk_w) {
    	    var_set_int_forkey (res_io, "wrops", delta_w / (ti - GLOB.lastrun));
    	}
	}

	delta = totalwait - GLOB.io_wait;
	cpudelta = totalcpu - GLOB.total_cpu;

	if (GLOB.io_wait)
	{
	    var *res_io = var_get_dict_forkey (res, "io");
	    var_set_double_forkey (res_io, "pwait", (100.0 * delta) / (1.0 * cpudelta));
	}

	GLOB.io_blk = totalblk;
	GLOB.io_wait = totalwait;
	GLOB.total_cpu = totalcpu;
	GLOB.lastrun = ti;
	
	return res;
}



typedef struct topentry_s {
	char				username[16];
	pid_t				pid;
	unsigned short		pcpu;
	unsigned short		pmem;
	time_t				secrun;
	char				ptitle[48];
} topentry;

#define NR_TPROCS 24
#define MAX_NTOP 16

typedef struct topinfo_s {
	short				 ntop;
	topentry        	 tprocs[NR_TPROCS];
} topinfo;

void sample_tprocs (procrun *run) {
	struct dirent	*de;
	DIR				*D;
	FILE			*F;
	pid_t			 pid;
	unsigned long	 utime;
	unsigned long	 stime;
	uid_t			 tuid;
	gid_t			 tgid;
	struct stat		 st;
	char			 buf[256];
	char			*c;
	wordlist		*words;
	struct timeval	 tv;
	int				 pausetimer;
	long long		 rss;
	long long		 tpmem;
	int				 pmem;
	
	procrun_initsample (run);
	D = opendir ("/proc");
	if (!D) return;
	pausetimer = 0;
	
	while ( (de = readdir (D)) ) {
		pid = atoi (de->d_name);
		
		/* Don't spike CPU on machines with lots of processes */
		pausetimer++;
		if (pausetimer & 15) {
			tv.tv_sec = 0;
			tv.tv_usec = 64;
			select (0, NULL, NULL, NULL, &tv);
		}
		
		if (pid>0) {
			sprintf (buf, "/proc/%d", pid);
			if (stat (buf, &st) == 0) {
				tuid = st.st_uid;
				tgid = st.st_gid;
			}
			else continue;

			sprintf (buf, "/proc/%d/stat", pid);
			if ( (F = fopen (buf, "r")) ) {
				fgets (buf, 255, F);
				buf[255] = 0;
				
				c = buf;
				while (strchr (c, ')')) {
					c = strchr (c, ')') + 1;
				}
					
				/* 11=u 12=s */
				words = wordlist_make (c);
				if (words->argc > 12) {
					utime = atol (words->argv[11]);
					stime = atol (words->argv[12]);
				}
				fclose (F);
				wordlist_free (words);

				sprintf (buf, "/proc/%d/statm", pid);
				if ((F = fopen (buf, "r"))) {
					memset (buf, 0, 255);
					fgets (buf, 255, F);
					c = strchr (buf, ' ');
					if (c) {
						++c;
						rss = 4 * atoll (c);
						
						tpmem = rss;
						tpmem *= 10000;
						tpmem /= KMEMTOTAL;
						pmem = (int) (tpmem & 0xffffffff);
					}
					fclose (F);
				}
				else {
					rss = 128000;
				}
				
				sprintf (buf, "/proc/%d/cmdline", pid);
				if ((F = fopen (buf, "r"))) {
					memset (buf, 0, 255);
					fread (buf, 0, 255, F);
					fclose (F);
					
					if (*buf == 0) {
						sprintf (buf, "/proc/%d/status", pid);
						if ((F = fopen (buf, "r"))) {
							memset (buf, 0, 255);
							fgets (buf, 255, F);
							c = strchr (buf, ':');
							if (! c) {
								fclose (F);
								continue;
							}
							++c;
							while (*c <= ' ') ++c;
							if (strlen (c)) {
								c[strlen(c)-1] = 0;
							}
							
							fclose (F);
							
							procrun_setproc (run, pid, utime, stime,
											 tuid, tgid, c, pmem);
						}
					}
					else {
						procrun_setproc (run, pid, utime, stime,
										 tuid, tgid, buf, pmem);
					}
				}
			}
		}
	}
	closedir (D);
}

void make_top_hole (topinfo *inf, int pos) {
	int tailsz;
	
	if (pos>(MAX_NTOP-2)) return;
	tailsz = ((MAX_NTOP-1) - pos);
	
	memmove (inf->tprocs + pos + 1,
			 inf->tprocs + pos,
			 tailsz * sizeof (topentry));
	
	if (inf->ntop < MAX_NTOP) inf->ntop++;
}

var *gather_tprocs (procrun *procs) {
    int i;
    int j;
    int inserted;
    struct passwd *pwd;
    topinfo inf;
    inf.ntop = 0;
    
    var *res = var_alloc();
    
    sample_tprocs (procs);
    procrun_calc (procs);
    
    var_set_double_forkey (res, "pcpu",(procs->pcpu*1.0)/255.0);
    for (i=0; i<procs->count; ++i) {
        inserted = 0;
        if (! procs->array[i].pid) continue;
        for (j=0; j<inf.ntop;++j) {
            if (((3*inf.tprocs[j].pcpu) + (inf.tprocs[j].pmem/2)) <
                ((3*procs->array[i].pcpu) + (procs->array[i].pmem/2))) {
                if ((procs->array[i].pcpu == 0) &&
                    (procs->array[i].pmem < 501)) continue;
                
                make_top_hole (&inf, j);
                pwd = getpwuid (procs->array[i].uid);
                if (pwd) {
                    strncpy (inf.tprocs[j].username, pwd->pw_name, 15);
                    inf.tprocs[j].username[15] = 0;
                }
                else {
                    sprintf (inf.tprocs[j].username, "#%d",
                             procs->array[i].uid);
                }
                inf.tprocs[j].pid = procs->array[i].pid;
                inf.tprocs[j].pcpu = procs->array[i].pcpu;
                inf.tprocs[j].pmem = procs->array[i].pmem;
                inf.tprocs[j].secrun = 0;
                strncpy (inf.tprocs[j].ptitle, procs->array[i].ptitle, 47);
                inf.tprocs[j].ptitle[47] = 0;
                inserted = 1;
                j = inf.ntop;
            }
        }
        
        if ((! inserted) && (inf.ntop < MAX_NTOP-2) &&
            (procs->array[i].pcpu || (procs->array[i].pmem > 500))) {
            j = inf.ntop;
            inf.ntop++;
            
            pwd = getpwuid (procs->array[i].uid);
            if (pwd) {
                strncpy (inf.tprocs[j].username, pwd->pw_name, 15);
                inf.tprocs[j].username[15] = 0;
            }
            else {
                sprintf (inf.tprocs[j].username, "#%d", procs->array[i].uid);
            }
            
            inf.tprocs[j].pid = procs->array[i].pid;
            inf.tprocs[j].pcpu = procs->array[i].pcpu;
            inf.tprocs[j].pmem = procs->array[i].pmem;
            inf.tprocs[j].secrun = 0;
            strncpy (inf.tprocs[j].ptitle, procs->array[i].ptitle, 47);
            inf.tprocs[j].ptitle[47] = 0;
            inserted = 1;
        }
    }
    
    var *vtop = var_get_array_forkey (res, "top");
    for (i=0; i<inf.ntop; ++i) {
        var *vproc = var_add_dict (vtop);
        var_set_int_forkey (vproc, "pid", inf.tprocs[i].pid);
        var_set_str_forkey (vproc, "user", inf.tprocs[i].username);
        var_set_double_forkey (vproc, "pcpu", inf.tprocs[i].pcpu / 100.0);
        var_set_double_forkey (vproc, "pmem", inf.tprocs[i].pmem / 100.0);
        var_set_str_forkey (vproc, "name", inf.tprocs[i].ptitle);
    }
    
    procrun_newround (procs);
    return res;
}

static struct topthreadinfo_s {
    var *oldres;
    var *res;
    thread *thread;
} TOPTHREAD = {NULL,NULL, NULL};

void run_top (thread *self) {
    FILE *F;
    char buf[256];
    
    procrun *p = (procrun *) malloc (sizeof (procrun));
    int i;
    
    procrun_init (p);

    F = fopen ("/proc/meminfo","r");
    if (F) {
        while (! feof (F)) {
            *buf = 0;
            fgets (buf, 255, F);
            
            if (strncmp (buf, "MemTotal:", 9) == 0) {
                KMEMTOTAL = atoi (buf+9);
            }
        }
        fclose (F);
    }

	F = fopen ("/proc/cpuinfo","r");
	if (F) {
		while (! feof (F)) {
			buf[0] = 0;
			fgets (buf, 255, F);
			if (strncasecmp (buf, "processor", 9) == 0) {
				p->ncpu++;
			}
		}
		fclose (F);
	}
	else {
		p->ncpu = 1;
	}

    while (1) {
        for (i=0; i<3; ++i) {
            sample_tprocs (p);
            sleep (10);
        }
        var *newres = gather_tprocs (p);
        if (TOPTHREAD.oldres) {
            var_free (TOPTHREAD.oldres);
            TOPTHREAD.oldres = NULL;
        }
        if (TOPTHREAD.res) TOPTHREAD.oldres = TOPTHREAD.res;
        TOPTHREAD.res = newres;
    }
}

var *runprobe_top (probe *self) {
    if (! TOPTHREAD.thread) {
        TOPTHREAD.thread = thread_create (run_top, NULL);
    }

    if (TOPTHREAD.res) {
        var *res = var_alloc();
        var_copy (res, TOPTHREAD.res);
        return res;
    }    
    return NULL;
}

builtinfunc BUILTINS[] = {
    GLOBAL_BUILTINS,
    {"probe_top", runprobe_top},
    {"probe_uptime", runprobe_uptime},
    {"probe_io", runprobe_io}
    {NULL, NULL}
};

