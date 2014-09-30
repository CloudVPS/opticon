#ifndef _UUID_H
#define _UUID_H 1

/** UUIDs are normally passed by value */
typedef struct { uint64_t msb; uint64_t lsb; } uuid;

int          uuidcmp (uuid first, uuid second);
uuid         mkuuid (const char *str);
uuid         uuidgen (void);
uuid         uuidnil (void);
int          uuidvalid (uuid);
void         uuid2str (uuid u, char *into);

#endif
