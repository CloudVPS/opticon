#include <datatypes.h>
#include <util.h>
#include <assert.h>
#include <stdio.h>
#include <auth.h>
#include <codec.h>
#include <compress.h>
#include <aes.h>

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
    meterid_t M_NET_NAME = makeid ("net/name",MTYPE_STR,0);
    meterid_t M_NET_IN_KBS = makeid ("net/in.kbs",MTYPE_INT,0);
    meterid_t M_NET_OUT_KBS = makeid ("net/out.kbs",MTYPE_INT,0);
    meterid_t M_NET_IN_PPS = makeid ("net/in.pps",MTYPE_INT,0);
    meterid_t M_NET_OUT_PPS = makeid ("net/out.pps",MTYPE_INT,0);
    meterid_t M_HOSTNAME = makeid ("hostname",MTYPE_STR,0);
    meterid_t M_UNAME = makeid ("uname",MTYPE_STR,0);

    fstring D_NET_NAME[2] = {{"eth0"},{"eth1"}};
    uint64_t D_NET_IN_KBS[2] = {100ULL,70ULL};
    uint64_t D_NET_OUT_KBS[2] = {250ULL, 90ULL};
    uint64_t D_NET_IN_PPS[2] = {150ULL,100ULL};
    uint64_t D_NET_OUT_PPS[2] = {250ULL,117ULL};
    
    meterid_t M_TOP_PID = makeid ("top/pid",MTYPE_INT,0);
    meterid_t M_TOP_USER = makeid ("top/user",MTYPE_STR,0);
    meterid_t M_TOP_PCPU = makeid ("top/pcpu",MTYPE_FRAC,0);
    meterid_t M_TOP_PMEM = makeid ("top/pmem",MTYPE_FRAC,0);
    meterid_t M_TOP_NAME = makeid ("top/name",MTYPE_STR,0);
    
    meterid_t M_MEM_SIZE = makeid ("mem/size",MTYPE_INT,0);
    meterid_t M_MEM_FREE = makeid ("mem/free",MTYPE_INT,0);
    meterid_t M_SWAP_SIZE = makeid ("swap/size",MTYPE_INT,0);
    meterid_t M_SWAP_FREE = makeid ("swap/free",MTYPE_INT,0);
    
    meterid_t M_PCPU = makeid ("pcpu",MTYPE_FRAC,0);
    meterid_t M_PIOWAIT = makeid ("piowait",MTYPE_FRAC,0);
    meterid_t M_LOADAVG = makeid ("loadavg",MTYPE_FRAC,0);
    
    uint64_t D_TOP_PID[11] = {
        27562,28074,27983,27560,28311,19082,2411,2082,6129,1862,2564
    };
    const char *D_TOP_USER[11] = {
        "www-data",
        "www-data",
        "www-data",
        "www-data",
        "www-data",
        "www-data",
        "root",
        "mysql",
        "root",
        "nobody",
        "root"
    };
    const double D_TOP_PCPU[11] = {
        16.92, 16.92, 16.88, 16.88, 16.88, 16.88, 0.03, 0.03, 0.00, 0.29, 0.03
    };
    const double D_TOP_PMEM[11] = {
        0.14,0.14,0.14,0.14,0.14,0.14,10.94,5.37,5.54,0.09,0.49
    };
    const char *D_TOP_NAME[11] = {
        "apache2",
        "apache2",
        "apache2",
        "apache2",
        "apache2",
        "apache2",
        "zarafa-server",
        "mysqld",
        "spamd",
        "n2txd",
        "zarafa-search"
    };
    
    tenant_init();
    sessionlist_init();
    
    tenant_create (tenantid, "test");
    tenant *T = tenant_find (tenantid);
    assert (T->uuid.lsb == 0x001b71534f4b4f1c);
    assert (T->uuid.msb == 0xb281cc06b134f98f);
    host *H = host_find (tenantid, hostid);
    host_begin_update (H);

    id2str (M_NET_IN_KBS, tstr);
    meter *M = host_get_meter (H, M_HOSTNAME);
    meter_setcount (M, 0);
    meter_set_str (M, 0, "srv-01.heikneuter.nl");
    
    M = host_get_meter (H, M_UNAME);
    meter_setcount (M, 0);
    meter_set_str (M, 0, "Linux srv-01.heikneuter.nl 2.6.18-prep #1 SMP");
    
    M = host_get_meter (H, M_PCPU);
    meter_setcount (M, 0);
    meter_set_frac (M, 0, 100.0);
    
    M = host_get_meter (H, M_PIOWAIT);
    meter_setcount (M, 0);
    meter_set_frac (M, 0, 3.0);
    
    M = host_get_meter (H, M_LOADAVG);
    meter_setcount (M, 0);
    meter_set_frac (M, 0, 6.03);
    
    M = host_get_meter (H, M_MEM_SIZE);
    meter_setcount (M, 0);
    meter_set_uint (M, 0, 524288);
    
    M = host_get_meter (H, M_MEM_FREE);
    meter_setcount (M, 0);
    meter_set_uint (M, 0, 424250);
    
    M = host_get_meter (H, M_SWAP_SIZE);
    meter_setcount (M, 0);
    meter_set_uint (M, 0, 1048576);
    
    M = host_get_meter (H, M_SWAP_FREE);
    meter_setcount (M, 0);
    meter_set_uint (M, 0, 122020);
    
    M = host_set_meter_str (H, M_NET_NAME, 2, D_NET_NAME);
    M = host_set_meter_uint (H, M_NET_IN_KBS, 2, D_NET_IN_KBS);
    M = host_set_meter_uint (H, M_NET_IN_PPS, 2, D_NET_IN_PPS);
    assert (meter_get_uint (M, 1) == 100ULL);
    M = host_set_meter_uint (H, M_NET_OUT_KBS, 2, D_NET_OUT_KBS);
    M = host_set_meter_uint (H, M_NET_OUT_PPS, 2, D_NET_OUT_PPS);

    M = host_get_meter (H, M_TOP_PID);
    meter_setcount (M, 11);
    for (i=0; i<11; ++i) meter_set_uint (M, i, D_TOP_PID[i]);
    
    M = host_get_meter (H, M_TOP_USER);
    meter_setcount (M, 11);
    for (i=0; i<11; ++i) meter_set_str (M, i, D_TOP_USER[i]);
    
    M = host_get_meter (H, M_TOP_PCPU);
    meter_setcount (M, 11);
    for (i=0; i<11; ++i) meter_set_frac (M, i, D_TOP_PCPU[i]);
    
    M = host_get_meter (H, M_TOP_PMEM);
    meter_setcount (M, 11);
    for (i=0; i<11; ++i) meter_set_frac (M, i, D_TOP_PMEM[i]);
    
    M = host_get_meter (H, M_TOP_NAME);
    meter_setcount (M, 11);
    for (i=0; i<11; ++i) meter_set_str (M, i, D_TOP_NAME[i]);
    
    aeskey key = aeskey_create();
    session *S = session_register (tenantid, hostid,
                                   0x0a000001, 0x31337666,
                                   key);
    
    S = session_find (0x0a000001, 0x31337666);
    assert (S != NULL);
    session_expire (time(NULL)+1);
    S = session_find (0x0a000001, 0x31337666);
    assert (S == NULL);
    
    char bouncebuf[4096];
    
    codec *C = codec_create_pkt();
    ioport *IO = ioport_create_buffer (bouncebuf, 4096);
    
    if (! codec_encode_host (C, IO, H)) {
        fprintf (stderr, "Encode failed\n");
        return 1;
    }
    
    bufferstorage *stor = (bufferstorage*) IO->storage;
    printf ("\n--> Encoded in %i bytes\n", stor->pos);
    size_t orig_size = stor->pos;
    
    char compressed[4096];
    ioport *CmpIO = ioport_create_buffer (compressed, 4096);
    compress_data (IO, CmpIO);
    printf ("--> Compressed %lu bytes\n", ioport_read_available (CmpIO));
    
    char encrypted[4096];
    ioport *CryptIO = ioport_create_buffer (encrypted, 4096);
    ioport_encrypt (&key, CmpIO, CryptIO, tnow);
    printf ("--> Encrypted %lu bytes\n", ioport_read_available (CryptIO));
    
    char decrypted[4096];
    ioport *DecrIO = ioport_create_buffer (decrypted, 4096);
    ioport_decrypt (&key, CryptIO, DecrIO, tnow);
    printf ("--> Decrypted %lu bytes\n", ioport_read_available (DecrIO));
    
    char decompressed[4096];
    ioport *DcmpIO = ioport_create_buffer (decompressed, 4096);
    decompress_data (DecrIO, DcmpIO);
    size_t bounce_size = ioport_read_available (DcmpIO);
    printf ("--> Decompressed %lu bytes\n\n", bounce_size);

    assert (bounce_size == orig_size);

    host *HH = host_alloc();
    host_begin_update (HH);
    if (! codec_decode_host (C, DcmpIO, HH)) {
        fprintf (stderr, "Decode failed\n");
        return 1;
    }
    
    ioport_close (DcmpIO);
    ioport_close (DecrIO);
    ioport_close (CryptIO);
    ioport_close (CmpIO);
    ioport_close (IO);
    codec_release (C);
    
    C = codec_create_json();
    IO = ioport_create_filewriter (stdout);
    codec_encode_host (C, IO, HH);
    ioport_close (IO);
    codec_release (C);
        
    M = host_get_meter (H, M_NET_IN_PPS);
    M = meter_next_sibling (M);
    assert (M != NULL);
    host_delete (H);
    host_delete (HH);
    return 0;
}
