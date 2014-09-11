#ifndef _AUTH_H
#define _AUTH_H 1

#include <datatypes.h>
#include <ioport.h>
#include <time.h>

/* =============================== TYPES =============================== */

/** Storage for an AES256 key */
typedef struct aeskey_s {
    uint8_t data[32];
} aeskey;

/** Representation of an active session */
typedef struct session_s {
    struct session_s    *next; /**< List link */
    struct session_s    *prev; /**< List link */
    uuid                 tenantid; /**< Tenant uuid associated */
    uuid                 hostid; /**< Host uuid associated */
    uint32_t             addr; /**< IP-derived id */
    uint32_t             sessid; /**< The sessionid */
    aeskey               key; /**< Session AES key */
    time_t               lastcycle; /**< Time since last key cycle */
    host                *host; /**< Connected host */
} session;

/** Linked list head for sessions */
typedef struct sessionlist_s {
    session             *first; /**< This bucket's first session */
    session             *last; /**< This bucket's last session */
} sessionlist;

/* ============================== GLOBALS ============================== */

extern sessionlist SESSIONS[256];

/* ============================= FUNCTIONS ============================= */

aeskey       aeskey_create (void);

void         sessionlist_init (void);
session     *session_alloc (void);
void         session_link (session *);

session     *session_register (uuid tenantid, uuid hostid, 
                               uint32_t addrpart, uint32_t sess_id,
                               aeskey sess_key);
                               
session     *session_find (uint32_t addr, uint32_t sess_id);
void         session_expire (time_t);
void         session_print (session *, ioport *);

#endif
