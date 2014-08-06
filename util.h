#ifndef _UTIL_H
#define _UTIL_H 1

#include <datatypes.h>

int uuidcmp (uuid first, uuid second);
uuid mkuuid (const char *str);
void id2str (meterid_t id, char *into);
meterid_t makeid (const char *label, metertype_t type, int pos);
void dump_host_json (host *, int);

#endif
