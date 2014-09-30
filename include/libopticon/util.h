#ifndef _UTIL_H
#define _UTIL_H 1

#include <libopticon/datatypes.h>
#include <libopticon/ioport.h>
#include <libopticon/uuid.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/socket.h>

/* ============================= FUNCTIONS ============================= */

void         id2str (meterid_t id, char *into);
void         nodeid2str (meterid_t id, char *into);
uint64_t     idhaspath (meterid_t);
meterid_t    makeid (const char *label, metertype_t type, int pos);
char        *load_file (const char *);
void         ip2str (struct sockaddr_storage *, char *);

#endif
