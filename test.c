#include <datatypes.h>
#include <util.h>
#include <assert.h>
#include <stdio.h>
#include <auth.h>

#define FD_STDOUT 1

int main (int argc, const char *argv[]) {
	const char *stenantid = "001b71534f4b4f1cb281cc06b134f98f";
	const char *shostid = "6f943a0d-bcd9-42fa-b0c9-6ede92f9a46a";
	char tstr[16];
	uuid tenantid = mkuuid (stenantid);
	uuid hostid = mkuuid (shostid);
	uint64_t kbpsdata[1] = {100ULL};
	uint64_t ppsdata[2] = {150ULL,100ULL};
	
	sessionlist_init();
	
	tenant_create (tenantid, "test");
	tenant *T = tenant_find (tenantid);
	assert (T->uuid.lsb == 0x001b71534f4b4f1c);
	assert (T->uuid.msb == 0xb281cc06b134f98f);
	host *H = host_find (tenantid, hostid);
	meterid_t meterid = makeid ("net.in.kbs",MTYPE_INT,0);
	id2str (meterid, tstr);
	meter *M = host_set_meter_uint (H, meterid, 0, kbpsdata);
	meterid = makeid ("net.in.pps",MTYPE_INT,0);
	M = host_set_meter_uint (H, meterid, 2, ppsdata);
	assert (meter_get_uint (M, 1) == 100ULL);
	meterid = makeid ("net.out.kbs",MTYPE_INT,0);
	M = host_set_meter_uint (H, meterid, 0, kbpsdata);
	meterid = makeid ("net.out.pps",MTYPE_INT,0);
	M = host_set_meter_uint (H, meterid, 2, ppsdata);
	
	aeskey key = aeskey_create();
	session *S = session_register (tenantid, hostid,
								   0x0a000001, 0x31337666,
								   key);
	
	S = session_find (0x0a000001, 0x31337666);
	assert (S != NULL);
	session_print (S, FD_STDOUT);
	session_expire (time(NULL)+1);
	S = session_find (0x0a000001, 0x31337666);
	assert (S == NULL);
	
	dump_host_json (H, FD_STDOUT);
}
