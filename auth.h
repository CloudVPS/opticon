#ifndef _AUTH_H
#define _AUTH_H 1

#include <datatypes.h>
#include <time.h>

typedef struct aeskey_s {
    uint8_t data[32];
} aeskey;

typedef struct session_s {
    struct session_s    *next;
    struct session_s    *prev;
    uuid                 tenantid;
    uuid                 hostid;
    uint32_t             addr;
    uint32_t             sessid;
    aeskey               key;
    time_t               lastcycle;
    host                *host;
} session;

typedef struct sessionlist_s {
    session             *first;
    session             *last;
} sessionlist;

extern sessionlist SESSIONS[256];

aeskey       aeskey_create (void);

void         sessionlist_init (void);
session     *session_alloc (void);
void         session_link (session *);

session     *session_register (uuid tenantid, uuid hostid, 
                               uint32_t addrpart, uint32_t sess_id,
                               aeskey sess_key);
                               
session     *session_find (uint32_t addr, uint32_t sess_id);
void         session_expire (time_t);

#endif
