#ifndef _PKTWRAP_H
#define _PKTWRAP_H 1

#include <stdlib.h>
#include <stddef.h>
#include <sys/socket.h>
#include <libopticon/ioport_buffer.h>
#include <libopticon/aes.h>

/* =============================== TYPES =============================== */

typedef aeskey *(*resolve_tenantkey_f)(uuid, uint32_t);
typedef aeskey *(*resolve_sessionkey_f)(uint32_t, uint32_t, uint32_t, void **);

#define UNWRAP_TYPE_ERROR 0x00000001
#define UNWRAP_KEYRESOLVE_ERROR 0x00000002
#define UNWRAP_DECRYPT_ERROR 0x00000003
#define UNWRAP_DECOMPRESS_ERROR 0x00000004
#define UNWRAP_READ_ERROR 0x0000005

extern int unwrap_errno;

/** Authentication information going in/out of an authentication packet. */
typedef struct authinfo_s {
    uint32_t    sessionid; /** Selected session-id */
    uint32_t    serial; /** Sent serial number */
    uuid        tenantid; /** Tenant UUID */
    uuid        hostid; /** Host UUID */
    aeskey      sessionkey; /** AES256 session key */
    aeskey      tenantkey; /** AES256 tenant key */
} authinfo;

/* ============================== GLOBALS ============================== */

extern uint32_t SERIAL;

/* ============================= FUNCTIONS ============================= */

uint32_t     gen_networkid (struct sockaddr_storage *);
uint32_t     gen_sessionid (void);
uint32_t     gen_serial (void);

ioport      *ioport_wrap_meterdata (uint32_t, uint32_t, aeskey, ioport *);
ioport      *ioport_wrap_authdata (authinfo *, uint32_t);
ioport      *ioport_unwrap_meterdata (uint32_t, ioport *,
                                      resolve_sessionkey_f,
                                      void **sessiondata);
authinfo    *ioport_unwrap_authdata (ioport *, resolve_tenantkey_f);

#endif
