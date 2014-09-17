#ifndef _CLIOPT_H
#define _CLIOPT_H 1

/* =============================== TYPES =============================== */

typedef int (*setoption_f)(const char *, const char *);

#define OPT_VALUE 0x01
#define OPT_FLAG 0x00
#define OPT_ISSET 0x02

typedef struct cliopt_s {
    const char  *longarg;
    const char  *shortarg;
    int          flag;
    const char  *defaultvalue;
    setoption_f  handler;
} cliopt;

typedef int (*clicmd_f)(int, const char **);

typedef struct clicmd_s {
    const char  *command;
    clicmd_f     func;
} clicmd;

/* ============================= FUNCTIONS ============================= */

const char **cliopt_dispatch (cliopt *, const char **, int *);
int          cliopt_runcommand (clicmd *, const char *, int, const char **);

#endif
