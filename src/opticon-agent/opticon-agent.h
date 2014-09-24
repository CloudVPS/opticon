#ifndef _OPTICON_AGENT_H
#define _OPTICON_AGENT_H 1

#include <libopticon/datatypes.h>
#include <libopticon/codec.h>
#include <libopticon/auth.h>
#include <libopticon/pktwrap.h>
#include <libopticonf/var.h>
#include <libsvc/transport.h>
#include <libsvc/thread.h>
#include <libsvc/packetqueue.h>

/* =============================== TYPES =============================== */

/** Types of metering probes */
typedef enum probetype_e {
    PROBE_NONE, /**< Uninitialized */
    PROBE_BUILTIN, /**< One of the compiled-in probes */
    PROBE_EXEC /**< An external process */
} probetype;

struct probe_s; /**< Forward declaration */

typedef var *(*probefunc_f)(struct probe_s *);

/** Represents a probe */
typedef struct probe_s {
    thread           thr; /**< Underlying thread */
    conditional      pulse; /**< Trigger to ask probe to run once */
    probetype        type; /**< The type */
    const char      *call; /**< String representation of probe call */
    probefunc_f      func; /**< Function that performs one probe */
    struct probe_s  *prev; /**< Link neighbour */
    struct probe_s  *next; /**< Link neighbour */
    var             *vcurrent; /**< Current value */
    var             *vold; /**< Previous value */
    time_t           lastpulse; /**< Last rigger time */
    time_t           lastreply; /**< Last update time */
    time_t           lastdispatch; /**< Last time it was dispatched */
    int              interval; /**< Configured time interval */
} probe;

/** List header for a collection of probe objects */
typedef struct probelist_s {
    probe           *first;
    probe           *last;
} probelist;

/** Static mapping of built-in probe names to their functions */
typedef struct builtinfunc_s {
    const char      *name;
    probefunc_f      func;
} builtinfunc;

typedef struct authresender_s {
    thread           super;
    conditional     *cond;
    outtransport    *trans;
    pktbuf           buf;
} authresender;   

/** Useful access to application parts and configuration */
typedef struct appcontext_s {
    codec           *codec;
    outtransport    *transport;
    authresender    *resender;
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

/* ============================== GLOBALS ============================== */

extern builtinfunc BUILTINS[]; /**< NULL-terminated map of built-in probes */
extern appcontext APP; /**< The keep-it-all-together blob */

/* ============================= FUNCTIONS ============================= */

probe           *probe_alloc (void);
void             probelist_add (probelist *, probetype, const char *, int);
void             probelist_start (probelist *);
void             probelist_cancel (probelist *);
authresender    *authresender_create (outtransport *);
void             authresender_schedule (authresender *, const void *, size_t);


#endif
