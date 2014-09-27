#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <libopticon/util.h>
#include <libopticon/datatypes.h>
#include <libopticon/auth.h>
#include <libopticon/base64.h>
#include <libopticon/log.h>

sessionlist SESSIONS[256];

/** Initialize session-related global storage. */
void sessionlist_init (void) {
    for (int i=0; i<256; ++i) {
        SESSIONS[i].first = SESSIONS[i].last = NULL;
    }
}

/** Allocate and initialize session object. */
session *session_alloc (void) {
    session *res = (session *) malloc (sizeof (session));
    res->next = res->prev = NULL;
    res->addr = res->sessid = res->lastserial = 0;
    bzero (res->key.data, 8);
    res->lastcycle = time (NULL);
    res->host = NULL;
    return res;
}

/** Link a new session into the proper bucket. */
void session_link (session *s) {
    uint32_t sid = s->sessid ^ s->addr;
    uint8_t bucket = (sid >> 24) ^
                     ((sid & 0x00ff0000) >> 16) ^
                     ((sid & 0x0000ff00) >> 8) ^
                     (sid & 0xff);
    if (SESSIONS[bucket].first) {
        // link in at top.
        s->next = SESSIONS[bucket].first;
        SESSIONS[bucket].first->prev = s;
        SESSIONS[bucket].first = s;
    }
    else {
        SESSIONS[bucket].first = SESSIONS[bucket].last = s;
    }
}

/** Register a new session (or update an existing session to
  * stop it from expiring. */
session *session_register (uuid tenantid, uuid hostid,
                           uint32_t addrpart, uint32_t sess_id,
                           aeskey sess_key) {
    host *h = host_find (tenantid, hostid);
    session *s;
    if (! h) return NULL;

    if ((s = session_find (addrpart, sess_id))) {
        s->lastcycle = time (NULL);
        s->key = sess_key;
        return s;
    }
        
    s = session_alloc();
    if (! s) return NULL;
    
    s->tenantid = tenantid;
    s->hostid = hostid;
    s->host = h;
    s->addr = addrpart;
    s->sessid = sess_id;
    s->key = sess_key;
    s->lastcycle = time (NULL);
    
    session_link (s);
    return s;
}

/** Look up a session by 32 bits worth of IP address data and a 32 bits
    session-id. */
session *session_find (uint32_t addrpart, uint32_t sess_id) {
    uint32_t sid = sess_id ^ addrpart;
    uint8_t bucket = (sid >> 24) ^
                     ((sid & 0x00ff0000) >> 16) ^
                     ((sid & 0x0000ff00) >> 8) ^
                     (sid & 0xff);
    session *s = SESSIONS[bucket].first;
    while (s) {
        if (s->addr == addrpart && s->sessid == sess_id) return s;
        s = s->next;
    }
    return NULL;
}

/** Expire sessions that have been last updated before a given
    cut-off point. */
void session_expire (time_t cutoff) {
    session *s, *ns;
    for (int i=0; i<=255; ++i) {
        s = SESSIONS[i].first;
        while (s) {
            ns = s->next;
            if (s->lastcycle < cutoff) {
                log_info ("Expiring session %08x-%08x",s->sessid,s->addr);
                if (s->prev) {
                    if (s->next) {
                        s->prev->next = s->next;
                        s->next->prev = s->prev;
                    }
                    else {
                        s->prev->next = NULL;
                        SESSIONS[i].last = s->prev;
                    }
                }
                else {
                    if (s->next) {
                        s->next->prev = NULL;
                        SESSIONS[i].first = s->next;
                    }
                    else {
                        SESSIONS[i].first = SESSIONS[i].last = NULL;
                    }
                }
                
                free (s);
            }
            s = ns;
        }
    }
}

/** Dump information about a session into a filedescriptor. */
void session_print (session *s, ioport *into) {
    char buf[256];
    char *keystr;
    char stenantid[48], shostid[48];
    uuid2str (s->tenantid, stenantid);
    uuid2str (s->hostid, shostid);
    keystr = base64_encode ((char *) s->key.data, 32, NULL);
    
    sprintf (buf, "Session %08x\n"
                  "Tenant %s\n"
                  "Host %s\n"
                  "Key %s\n",
                  s->sessid,
                  stenantid,
                  shostid,
                  keystr);
    
    free (keystr);
    ioport_write (into, buf, strlen (buf));
}
