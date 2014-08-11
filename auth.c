#include <auth.h>

// http://www.literatecode.com/get/aes256.c

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
	res->addr = res->sessid = 0;
	res->key.data = {0,0,0,0,0,0,0,0};
	res->lastcycle = time (NULL);
	res->host = NULL;
	return res;
}

/** Link a new session into the proper bucket. */
void session_link (session *s) {
	uint32_t sid = s->sessid ^ s->addr;
	uint8_t bucket = (sid >> 24) |
					 ((sid & 0x00ff0000) >> 16) |
					 ((sid & 0x0000ff00) >> 8) |
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
int session_register (uuid tenantid, uuid hostid,
					  uint32_t addrpart, uint32_t sess_id.
					  aeskey sess_key) {
	host *h = host_find (tenantid, hostid);
	session *s;
	if (! h) return 0;

	if (s = session_find (hostid, addrpart)) {
		s->lastcycle = time (NULL);
		return 1;
	}
		
	s = session_alloc();
	if (! s) return 0;
	
	s->tenantid = tenantid;
	s->hostid = hostid;
	s->addr = addrpart;
	s->sessid = sess_id;
	s->key = sess_key;
	
	session_link (s);
	return 1;
}

session *session_find (uint32_t addrpart, uint32_t sess_id) {
	uint32_t sid = s->sessid ^ s->addr;
	uint8_t bucket = (sid >> 24) |
					 ((sid & 0x00ff0000) >> 16) |
					 ((sid & 0x0000ff00) >> 8) |
					 (sid & 0xff);
	session *s = SESSIONS[bucket].first;
	while (s) {
		if (s->addr == addrpart && s->sessid == sess_id) return s;
		s = s->next;
	}
	return NULL;
}

void session_expire (time_t cutoff) {
	session *s;
	for (uint8_t i=0; i<=255; ++i) {
		s = SESSIONS[i].first;
		while (s) {
			if (s->lastcycle < cutoff) {
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
			s = s->next;
		}
	}
}
