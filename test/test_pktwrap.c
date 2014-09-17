#include <libopticon/pktwrap.h>
#include <libopticon/aes.h>
#include <libopticon/util.h>
#include <assert.h>
#include <stdio.h>

aeskey TENANTKEY;
aeskey SESSIONKEY;

aeskey *resolve_tenantkey (uuid tenant, uint32_t serial) {
    return &TENANTKEY;
}

#define NETID 0x23694205

aeskey *resolve_sessionkey (uint32_t netid, uint32_t sessionid,
                            uint32_t serial) {
    return &SESSIONKEY;
}

int main (int argc, const char *argv[]) {
    TENANTKEY = aeskey_create();
    SESSIONKEY = aeskey_create();
    const char *stenantid = "001b71534f4b4f1cb281cc06b134f98f";
    const char *shostid = "6f943a0d-bcd9-42fa-b0c9-6ede92f9a46a";
    uuid tenantid = mkuuid (stenantid);
    uuid hostid = mkuuid (shostid);
    authinfo auth;
    auth.sessionid = gen_sessionid();
    auth.tenantid = tenantid;
    auth.hostid = hostid;
    auth.tenantkey = TENANTKEY;
    auth.sessionkey = SESSIONKEY;
    
    ioport *auth_port;
    assert (auth_port = ioport_wrap_authdata (&auth, 123));
    
    authinfo *resauth;
    assert (resauth = ioport_unwrap_authdata (auth_port, resolve_tenantkey));
    assert (uuidcmp (resauth->tenantid, tenantid));
    assert (uuidcmp (resauth->hostid, hostid));
    assert (resauth->sessionid = auth.sessionid);
    
    char worldbuf[256];
    
    ioport *outdata = ioport_create_buffer ((char*)"Hello, world.", 14);
    ioport *wrapped;
    ioport *unwrapped;
    
    assert (wrapped = ioport_wrap_meterdata (auth.sessionid, 124,
                                             auth.sessionkey, outdata));
    
    assert (unwrapped = ioport_unwrap_meterdata (NETID, wrapped,
                                                 resolve_sessionkey));
    assert (ioport_read (unwrapped, worldbuf, 14));
    assert (strcmp (worldbuf, "Hello, world.") == 0);
}
