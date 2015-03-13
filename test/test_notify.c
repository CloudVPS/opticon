#include <libopticon/datatypes.h>
#include <libopticon/util.h>
#include <libopticon/auth.h>
#include <libopticon/codec_pkt.h>
#include <libopticon/codec_json.h>
#include <libopticon/compress.h>
#include <libopticon/ioport_file.h>
#include <libopticon/aes.h>
#include <libopticon/log.h>
#include <assert.h>
#include <stdio.h>

#define FD_STDOUT 1

void hexdump_buffer (ioport *io) {
    size_t sz = ioport_read_available (io);
    size_t i;
    char *buf = ioport_get_buffer (io);
    if (! buf) return;
    
    for (i=0; i<sz; ++i) {
        printf ("%02x ", (uint8_t) buf[i]);
        if ((i&31) == 31) printf ("\n");
    }
    if (i&31) printf ("\n");
}

int main (int argc, const char *argv[]) {
    const char *stenantid = "001b71534f4b4f1cb281cc06b134f98f";
    const char *shostid = "6f943a0d-bcd9-42fa-b0c9-6ede92f9a46a";
    char tstr[16];
    time_t tnow = time (NULL);
    int i;
    uuid tenantid = mkuuid (stenantid);
    uuid hostid = mkuuid (shostid);
    tenant_init();
    sessionlist_init();
    
    aeskey tenantkey = aeskey_create();
    tenant *T = tenant_create (tenantid, tenantkey);
    tenant_done (T);
    
    T = tenant_find (tenantid, TENANT_LOCK_WRITE);
    assert (T->uuid.msb == 0x001b71534f4b4f1c);
    assert (T->uuid.lsb == 0xb281cc06b134f98f);

    tenant_set_notification (T, true, "test", "ALERT", hostid);
    assert (notifylist_check_actionable (&T->notify) == false);
    
    /* hack timing */
    assert (T->notify.first);
    T->notify.first->lastchange = time (NULL) - 300;
    
    assert (notifylist_check_actionable (&T->notify));
    
    tenant_delete (T);
    return 0;
}
