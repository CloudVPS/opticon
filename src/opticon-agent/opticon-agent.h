#ifndef _OPTICON_AGENT_H
#define _OPTICON_AGENT_H 1

#include <libopticon/datatypes.h>
#include <libopticon/codec.h>
#include <libopticon/auth.h>
#include <libopticon/pktwrap.h>
#include <libopticonf/var.h>
#include <libsvc/transport.h>
#include <libsvc/thread.h>

typedef enum probetype_e {
    PROBE_NONE,
    PROBE_BUILTIN,
    PROBE_EXEC
} probetype;

struct probe_s; /**< Forward declaration */

typedef var *(*probefunc_f)(struct probe_s *);

typedef struct probe_s {
    thread           thr;
    conditional      pulse;
    probetype        type;
    const char      *call;
    probefunc_f      func;
    struct probe_s  *prev;
    struct probe_s  *next;
    var             *vcurrent;
    var             *vold;
    time_t           lastpulse;
    time_t           lastreply;
    int              interval;
} probe;

typedef struct probelist_s {
    probe           *first;
    probe           *last;
} probelist;

typedef struct builtinfunc_s {
    const char      *name;
    probefunc_f      func;
} builtinfunc;


/** Useful access to application parts and configuration */
typedef struct appcontext_s {
    codec           *codec;
    outtransport    *transport;
    probelist        probes;
    var             *conf;
    const char      *logpath;
    const char      *confpath;
    const char      *pidfile;
    int              foreground;
    int              collectorport;
    const char      *collectoraddr;
    aeskey           collectorkey;
    uuid             tenantid;
    uuid             hostid;
    authinfo         auth;
} appcontext;

extern builtinfunc BUILTINS[];
extern appcontext APP;

probe   *probe_alloc (void);
void     probelist_add (probelist *, probetype, const char *, int);
void     probelist_start (probelist *);
void     probelist_cancel (probelist *);

#endif
