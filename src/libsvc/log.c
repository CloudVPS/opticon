#include <libsvc/log.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

loghandle *LOG = NULL;

/** Implementation of log_write */
void syslog_write (loghandle *h, int prio, const char *dt) {
    syslog (prio, "%s", dt);
}

typedef struct logfile_data_s {
    FILE *f;
    pthread_mutex_t mutex;
} logfile_data;

/** Implementaton of log_write for file-based logging */
void logfile_write (loghandle *h, int prio, const char *dt) {
    logfile_data *d = (logfile_data *) h->data;
    const char *leveltext = NULL;
    FILE *f = (FILE *) d->f;
    char tstr[64];

    switch (prio) {
        case LOG_ERR:
            leveltext = "ERROR";
            break;
        
        case LOG_WARNING:
            leveltext = "WARN ";
            break;
        
        case LOG_DEBUG:
            leveltext = "DEBUG";
            break;
        
        case LOG_INFO:
            leveltext = "INFO ";
            break;
        
        default:
            leveltext = "OTHER";
            break;
    }

    time_t tnow = time (NULL);
    struct tm tm;
    localtime_r (&tnow, &tm);
    strftime (tstr, 64, "%Y-%m-%d %H:%M:%S", &tm);
    
    pthread_mutex_lock (&d->mutex);
    fprintf (f, "%s [%s] %s\n", tstr, leveltext, dt);
    fflush (f);
    pthread_mutex_unlock (&d->mutex);
}

/** Connect to syslog
  * \param name Name of our program.
  */
void log_open_syslog (const char *name, int maxprio) {
    LOG = (loghandle *) malloc (sizeof (loghandle));
    LOG->write = syslog_write;
    LOG->maxprio = maxprio;
    LOG->data = NULL;
    openlog (name, LOG_PID, LOG_DAEMON);
}

/** Open a file-based logger.
  * \param filename Full path to the file to write/append to.
  */
void log_open_file (const char *filename, int maxprio) {
    LOG = (loghandle *) malloc (sizeof (loghandle));
    LOG->data = malloc (sizeof (logfile_data));
    logfile_data *d = (logfile_data *) LOG->data;
    
    d->f = fopen (filename, "a");
    pthread_mutex_init (&d->mutex, NULL);
    LOG->write = logfile_write;
    LOG->maxprio = maxprio;
}

/** Dispatcer of a message to either stderr, or the log handle. */
void log_string (int prio, const char *str) {
    if (LOG) {
        if (prio <= LOG->maxprio) LOG->write (LOG, prio, str);
    }
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

/** Write a log message at the LOG_DEBUG level */
void log_debug (const char *fmt, ...) {
    char buffer[4096];
    buffer[0] = 0;
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buffer, 4096, fmt, ap);
    va_end (ap);
    log_string (LOG_DEBUG, buffer);
}

/** Write a log message at the LOG_INFO level */
void log_info (const char *fmt, ...) {
    char buffer[4096];
    buffer[0] = 0;
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buffer, 4096, fmt, ap);
    va_end (ap);
    log_string (LOG_INFO, buffer);
}

/** Write a log message at the LOG_WARNING level */
void log_warn (const char *fmt, ...) {
    char buffer[4096];
    buffer[0] = 0;
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buffer, 4096, fmt, ap);
    va_end (ap);
    log_string (LOG_WARNING, buffer);
}

/** Write a log message at the LOG_ERR level */
void log_error (const char *fmt, ...) {
    char buffer[4096];
    buffer[0] = 0;
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buffer, 4096, fmt, ap);
    va_end (ap);
    log_string (LOG_ERR, buffer);
}
