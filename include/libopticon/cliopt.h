#ifndef _CLIOPT_H
#define _CLIOPT_H 1

/* =============================== TYPES =============================== */

typedef int (*setoption_f)(const char *, const char *);

#define OPT_VALUE 0x01 /**< Indicates option needs a value */
#define OPT_FLAG 0x00 /**< Indicates option acts as a flag */
#define OPT_ISSET 0x02 /**< Indicates whether option has been invoked */

/** Structure for matching command line options. */
typedef struct cliopt_s {
    const char  *longarg; /**< --long-version of option */
    const char  *shortarg; /**< -s (hort version) */
    int          flag; /**< flags */
    const char  *defaultvalue; /**< default value to set if none provided */
    setoption_f  handler; /**< Function to call to set the flag or value */
} cliopt;

typedef int (*clicmd_f)(int, const char **);

/** Structure for looking up a subcommand and calling a specific
  * function to handle it. */
typedef struct clicmd_s {
    const char  *command; /**< The command */
    clicmd_f     func; /**< The function to call */
} clicmd;

/* ============================= FUNCTIONS ============================= */

const char **cliopt_dispatch (cliopt *, const char **, int *);
int          cliopt_runcommand (clicmd *, const char *, int, const char **);

#endif
