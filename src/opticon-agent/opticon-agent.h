#ifndef _OPTICON_AGENT_H
#define _OPTICON_AGENT_H 1

typedef enum probetype_e {
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
    struct probe_s  *prev;
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


extern probelist PROBES;
extern probefunc BUILTINS[];

probe   *probe_alloc (void);
void     probelist_add (probelist *, probetype, const char *, int);
void     probelist_start (probelist *);
void     probelist_cancel (probelist *);

#endif
