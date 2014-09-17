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

/* ============================= FUNCTIONS ============================= */

const char **cliopt_dispatch (cliopt *, const char **, int *);

#endif
