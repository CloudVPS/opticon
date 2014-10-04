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
meterid_t    id2mask (meterid_t);
int          idisprefix (meterid_t potential, meterid_t prefixfor,
                         meterid_t mask);
meterid_t    idgetprefix (meterid_t);
char        *load_file (const char *);
void         ip2str (struct sockaddr_storage *, char *);

char        *time2str (time_t);
char        *time2utcstr (time_t);
time_t       str2time (const char *);
time_t       utcstr2time (const char *);

#endif
