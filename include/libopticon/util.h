#ifndef _UTIL_H
#define _UTIL_H 1

#include <libopticon/datatypes.h>
#include <libopticon/ioport.h>

/* ============================= FUNCTIONS ============================= */

int          uuidcmp (uuid first, uuid second);
uuid         mkuuid (const char *str);
void         uuid2str (uuid u, char *into);
void         id2str (meterid_t id, char *into);
void         nodeid2str (meterid_t id, char *into);
uint64_t     idhaspath (meterid_t);
meterid_t    makeid (const char *label, metertype_t type, int pos);

#endif