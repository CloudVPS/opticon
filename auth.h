#ifndef _AUTH_H
#define _AUTH_H 1

#include <datatypes.h>

typedef struct aeskey_s {
	uint8_t data[8];
} aeskey;

typedef struct session_s {
	struct session_s	*next;
	struct session_s	*prev;
	uuid				 tenantid;
	uuid				 hostid;
	uint32_t			 addr;
	uint32_t			 sessid;
	aeskey				 key;
	time_t				 lastcycle;
	host				*host;
} session;

typedef struct sessionlist_s {
	session				*first;
	session				*last;
} sessionlist;

extern sessionlist SESSIONS:

void		 sessionlist_init (void);
session		*session_alloc (void);
void		 session_link (session *);

void		 register_session (uuid tenantid, uuid hostid, 
							   uint32_t addrpart, uint32_t sess_id,
					 		   aeskey sess_key);
aeskey		 get_session_key (uint32_t addr, uint32_t sess_id);

#endif
