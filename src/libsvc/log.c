#include <libsvc/log.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

loghandle *LOG = NULL;

/** Implementation of log_write */
void syslog_write (int prio, const char *dt) {
    syslog (prio, "%s", dt);
}

/** Connect to syslog
  * \param name Name of our program.
  */
void log_open_syslog (const char *name) {
    LOG = (loghandle *) malloc (sizeof (loghandle));
    LOG->write = syslog_write;
    openlog (name, LOG_PID, LOG_DAEMON);
}

/** Dispatcer of a message to either stderr, or the log handle. */
void log_string (int prio, const char *str) {
    if (LOG) LOG->write (prio, str);
    else {
        if (prio == LOG_ERR) {
            fprintf (stderr, "[ERROR] ");
        }
        else if (prio == LOG_DEBUG) {
            fprintf (stderr, "[DEBUG] ");
        }
        else if (prio == LOG_WARNING) {
            fprintf (stderr, "[WARN ] ");
        }
        else fprintf (stderr, "[INFO ] ");
        fprintf (stderr, "%s\n", str);
    }
}

/** Write a log message at the LOG_INFO level */
void log_info (const char *fmt, ...) {
    char buffer[4096];
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buffer, 4096, fmt, ap);
    va_end (ap);
    log_string (LOG_INFO, buffer);
}

/** Write a log message at the LOG_WARNING level */
void log_warn (const char *fmt, ...) {
    char buffer[4096];
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buffer, 4096, fmt, ap);
    va_end (ap);
    log_string (LOG_WARNING, buffer);
}

/** Write a log message at the LOG_ERR level */
void log_error (const char *fmt, ...) {
    char buffer[4096];
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buffer, 4096, fmt, ap);
    va_end (ap);
    log_string (LOG_ERR, buffer);
}
