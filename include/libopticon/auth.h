#ifndef _AUTH_H
#define _AUTH_H 1

#include <libopticon/datatypes.h>
#include <libopticon/ioport.h>
#include <libopticon/aes.h>
#include <libopticon/var.h>
#include <time.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/socket.h>

/* =============================== TYPES =============================== */

/** Representation of an active session */
typedef struct session_s {
    struct session_s        *next; /**< List link */
    struct session_s        *prev; /**< List link */
    uuid                     tenantid; /**< Tenant uuid associated */
    uuid                     hostid; /**< Host uuid associated */
    uint32_t                 addr; /**< IP-derived id */
    uint32_t                 sessid; /**< The sessionid */
    uint32_t                 lastserial; /**< Serial# of last packet */
    aeskey                   key; /**< Session AES key */
    time_t                   lastcycle; /**< Time since last key cycle */
    struct sockaddr_storage  remote; /**< Last sen remote host */
    host                    *host; /**< Connected host */
} session;

/** Linked list head for sessions */
typedef struct sessionlist_s {
    session             *first; /**< This bucket's first session */
    session             *last; /**< This bucket's last session */
} sessionlist;

/* ============================== GLOBALS ============================== */

extern sessionlist SESSIONS[256];

/* ============================= FUNCTIONS ============================= */

void         sessionlist_init (void);
var         *sessionlist_save (void);
void         sessionlist_restore (var *);
session     *session_alloc (void);
void         session_link (session *);

session     *session_register (uuid tenantid, uuid hostid, 
                               uint32_t addrpart, uint32_t sess_id,
                               aeskey sess_key, struct sockaddr_storage *);
                               
session     *session_find (uint32_t addr, uint32_t sess_id);
void         session_expire (time_t);
void         session_print (session *, ioport *);

#endif
