#include <datatypes.h>
#include <util.h>
#include <assert.h>
#include <stdio.h>

int main (int argc, const char *argv[]) {
	const char *stenantid = "001b71534f4b4f1cb281cc06b134f98f";
	const char *shostid = "6f943a0d-bcd9-42fa-b0c9-6ede92f9a46a";
	uuid_t tenantid = mkuuid (stenantid);
	uuid_t hostid = mkuuid (shostid);
	uint64_t meterdata[1] = {100ULL};
	
	tenant_create (tenantid, "test");
	tenant *T = tenant_find (tenantid);
	assert (T->uuid.lsb == 0x001b71534f4b4f1c);
	assert (T->uuid.msb == 0xb281cc06b134f98f);
	host *H = host_find (tenantid, hostid);
	meterid_t meterid = makeid ("net.i.kbps",MTYPE_INT,0);
	meter *M = host_set_meter_uint (H, meterid, 0, meterdata);
	meterid = makeid ("net.i.pps",MTYPE_INT,0);
	M = host_set_meter_uint (H, meterid, 0, meterdata);
}