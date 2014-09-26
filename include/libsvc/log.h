#ifndef _LOG_H
#define _LOG_H 1

/* =============================== TYPES =============================== */

struct loghandle_s; /* forward declaration */

typedef void (*logwrite_f)(struct loghandle_s *, int, const char *);

/** Implementation handle for the logging system. */
typedef struct loghandle_s {
    logwrite_f       write; /**< Method */
    void            *data; /**< Implementation data */
    int              maxprio; /**< Maximum priority to log */
} loghandle;

/* ============================== GLOBALS ============================== */

extern loghandle *LOG; /**< Log system to use, NULL for stderr */

/* ============================= FUNCTIONS ============================= */

void log_open_syslog (const char *name, int maxprio);
void log_open_file (const char *filename, int maxprio);
void log_debug (const char *fmt, ...);
void log_info (const char *fmt, ...);
void log_error (const char *fmt, ...);
void log_warn (const char *fmt, ...);

#endif
