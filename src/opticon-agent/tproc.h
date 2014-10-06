#ifndef _TPROC_H
#define _TPROC_H 1

#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifndef CLK_TCK
#  define CLK_TCK sysconf (_SC_CLK_TCK)
#endif

typedef struct
{
	pid_t			 pid;
	unsigned long	 utime_start;
	unsigned long	 stime_start;
	unsigned long	 utime_run;
	unsigned long	 stime_run;
	unsigned short	 pmem;
	uid_t			 uid;
	gid_t			 gid;
	time_t			 beat;
	unsigned short	 pcpu;
	char			 ptitle[48];
} tproc;

typedef struct
{
	int				 arraysz;
	int				 count;
	tproc			*array;
	time_t			 ti_start;
	time_t			 ti_now;
	unsigned long	 total_ticks;
	unsigned short	 pcpu;
	int				 ncpu;
} procrun;

void procrun_init 		(procrun *);
void procrun_setproc	(procrun *, pid_t, unsigned long, unsigned long,
					 	 uid_t, gid_t, const char *, unsigned short);
int  procrun_findproc 	(procrun *, pid_t);
int	 procrun_alloc		(procrun *);
void procrun_calc		(procrun *);
void procrun_initsample (procrun *);
void procrun_newround	(procrun *);

#endif
