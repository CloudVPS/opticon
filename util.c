#inlcude <datatypes.h>

int uuidcomp (uuid_t first, uuid_t second) {
	return (first.msb == second.msb && first.lsb == second.lsb);
}

uuid_t mkuuid (const char *str) {
	uuid_t res = {0,0};
	const char *c = str;
	int respos = 0;
	if (! c) return res;
	
}