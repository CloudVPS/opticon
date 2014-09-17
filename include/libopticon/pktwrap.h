#ifndef _PKTWRAP_H
#define _PKTWRAP_H 1

#include <stdlib.h>
#include <stddef.h>
#include <sys/socket.h>
#include <libopticon/ioport_buffer.h>

typedef aeskey *(*resolve_tenantkey_f)(uuid, uint32_t);
typedef aeskey *(*resolve_sessionkey_f)(uint32_t, uint32_t, uint32_t);

typedef struct authinfo_s {
    uint32_t    sessionid;
    uuid        tenantid;
    uuid        hostid;
    aeskey      sessionkey;
    aeskey      tenantkey;
} authinfo;

extern uint32_t SERIAL;

uint32_t     gen_networkid (struct sockaddr_storage *);
uint32_t     gen_sessionid (void);
uint32_t     gen_serial (void);

ioport      *ioport_wrap_meterdata (uint32_t, uint32_t, aeskey, ioport *);
ioport      *ioport_wrap_authdata (authinfo *, uint32_t);
ioport      *ioport_unwrap_meterdata (uint32_t, ioport *,
                                      resolve_sessionkey_f);
authinfo    *ioport_unwrap_authdata (ioport *, resolve_tenantkey_f);

#endif
