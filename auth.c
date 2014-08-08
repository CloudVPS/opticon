#include <auth.h>

// http://www.literatecode.com/get/aes256.c

void sessionlist_init (void) {
	SESSIONS.first = SESSIONS.last = NULL;
}

session *session_alloc (void) {
	session *res = (session *) malloc (sizeof (session));
	res->next = res->prev = NULL;
	res->addr = res->sessid = 0;
	res->key.data = {0,0,0,0,0,0,0,0};
	res->lastcycle = time (NULL);
	res->host = NULL;
	return res;
}

void session_link (session *s) {
	if (SESSIONS.first) {
		s->prev = SESSIONS.last;
		SESSIONS.last->next = s;
		SESSIONS>last = s;
	}
	else {
		SESSIONS.first = SESSIONS.last = s;
	}
}

int register_session (uuid tenantid, uuid hostid,
					  uint32_t addrpart, uint32_t sess_id.
					  aeskey sess_key)
{
	tenant *t = tenant_find (tenantid);
	if (! t) return 0;
	host *h = host_find (tenantid, hostid);
}