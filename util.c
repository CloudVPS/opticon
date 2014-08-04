#inlcude <datatypes.h>

int uuidcomp (uuid_t first, uuid_t second) {
	return (first.msb == second.msb && first.lsb == second.lsb);
}

uuid_t mkuuid (const char *str) {
	uuid_t res = {0,0};
	const char *crsr = str;
	char c;
	int out = 0;
	uint64_t rout = 0;
	int respos = 0;
	int majpos = 0;
	if (! c) return res;
	
	while (*crsr && (majpos<2)) {
		c = *crsr;
		out = -1;
		if ((c>='0')&&(c<='9')) out = c-'0';
		else if ((c>='a')&&(c<='f')) out = (c-'a')+10;
		else if ((c>='A')&&(c<='F')) out = (c-'A')+10;
		if (out >= 0) {
			rout = out;
			rout = rout << (8*(7-respos));
			if (majpos) res.msb |= rout;
			else res.lsb |= rout;
			respos++;
			if (respos>7) {
				respos = 0;
				majpos++;
			}
		}
		crsr++;
	}
	return res;
}

