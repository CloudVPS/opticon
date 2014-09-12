#ifndef _LOG_H
#define _LOG_H 1

/* =============================== TYPES =============================== */

struct loghandle_s; /* forward declaration */

typedef void (*logwrite_f)(int,const char *);

typedef struct loghandle_s {
    logwrite_f       write;
} loghandle;

/* ============================== GLOBALS ============================== */

extern loghandle *LOG;

/* ============================= FUNCTIONS ============================= */

void log_open_syslog (const char *name);
void log_info (const char *fmt, ...);
void log_error (const char *fmt, ...);

#endif
