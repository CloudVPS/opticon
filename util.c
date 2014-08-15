#include <datatypes.h>
#include <util.h>
#include <stdio.h>
#include <unistd.h>

/** Compare two UUIDs.
  * \param first First UUID.
  * \param second Second UUID.
  * \return 0 if they differ, 1 if they're the same.
  */
int uuidcmp (uuid first, uuid second) {
	return (first.msb == second.msb && first.lsb == second.lsb);
}

/** Turn a UUID string into a UUID value.
  * \param str The input string
  * \return The UUID value.
  */
uuid mkuuid (const char *str) {
	uuid res = {0,0};
	const char *crsr = str;
	char c;
	int out = 0;
	uint64_t rout = 0;
	int respos = 0;
	int majpos = 0;
	if (! crsr) return res;
	
	while (*crsr && (majpos<2)) {
		c = *crsr;
		out = -1;
		if ((c>='0')&&(c<='9')) out = c-'0';
		else if ((c>='a')&&(c<='f')) out = (c-'a')+10;
		else if ((c>='A')&&(c<='F')) out = (c-'A')+10;
		if (out >= 0) {
			rout = out;
			rout = rout << (4*(15-respos));
			if (majpos) res.msb |= rout;
			else res.lsb |= rout;
			respos++;
			if (respos>15) {
				respos = 0;
				majpos++;
			}
		}
		crsr++;
	}
	return res;
}

/* 5 bit character mapping for ids */
const char *IDTABLE = " abcdefghijklmnopqrstuvwxyz.-_/@";
uint64_t ASCIITABLE[] = {
	// 0x00
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 0x10
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 0x20
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28, 27, 30,
	// 0x30
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 0x40
	31, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	// 0x50
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 0, 0, 0, 0, 29,
	// 0x60
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	// 0x70
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 0, 0, 0, 0, 0
};

/** Construct a meterid.
  * \param label The meter's label (max 11 characters).
  * \param type The meter's type (MTYPE_INT, etc).
  * \return A meterid_t value.
  */
meterid_t makeid (const char *label, metertype_t type, int pos) {
	meterid_t res = (meterid_t) type;
	int bshift = 56;
	const char *crsr = label;
	char c;
	while ((c = *crsr) && bshift > 1) {
		if (c>0) {
			res |= (ASCIITABLE[c] << bshift);
			bshift -= 5;
		}
		crsr++;
	}
	return res;
}

/** Extract an ASCII name from a meterid_t value.
  * \param id The meterid to parse.
  * \param into String to copy the ASCII data into (minimum size of 12).
  */
void id2str (meterid_t id, char *into) {
	char *out = into;
	char *end = into;
	char t;
	uint64_t tmp;
	int bshift = 56;
	*out = 0;
	while (bshift > 1) {
		tmp = ((id & MMASK_NAME) >> bshift) & 0x1f;
		t = IDTABLE[tmp];
		*out = t;
		if (t != ' ') end = out;
		out++;
		bshift -= 5;
	}
	*(end+1) = 0;
}

/** Write out a UUID to a string in ASCII 
  * \param u The UUID
  * \param into String buffer for the result, should fit 36 characters plus
  *             nul-terminator.
  */
void uuid2str (uuid u, char *into) {
	sprintf (into, "%08llx-%04llx-%04llx-"
				   "%04llx-%012llx",
				   ((u.lsb & 0xffffffff00000000) >> 32),
				   ((u.lsb & 0x00000000ffff0000) >> 16),
				   ((u.lsb & 0x000000000000ffff)),
				   ((u.msb & 0xffff000000000000) >> 48),
				   ((u.msb & 0x0000ffffffffffff)));
}

/** Write out a meter value in JSON value format.
  * \param type The metertype to read and encode
  * \param m The meter
  * \param pos The array position within the meter
  * \param outfd The file descriptor to write to
  */
void dump_value (metertype_t type, meter *m, int pos, int outfd) {
	char buf[1024];
	switch (type) {
		case MTYPE_INT:
			sprintf (buf, "%llu", m->d.u64[pos]);
			break;
		
		case MTYPE_FRAC:
			sprintf (buf, "%f", m->d.frac[pos]);
			break;
		
		case MTYPE_STR:
			sprintf (buf, "\"%s\"", m->d.str);
			break;
			
		default:
			buf[0] = '\0';
			break;
	}
	write (outfd, buf, strlen (buf));
}

/** Write out a host's state as JSON data.
  * \param h The host object
  * \param outfd File descriptor to write to
  */
void dump_host_json (host *h, int outfd) {
	char buffer[256];
	meter *m = h->first;
	int i;
	int first=1;
	while (m) {
		if (first) first=0;
		else write (outfd, ",\n", 2);
		write (outfd, "\"", 1);
		id2str (m->id, buffer);
		write (outfd, buffer, strlen(buffer));
		write (outfd, "\":", 2);
		if (m->count > 0) {
			write (outfd, "[", 1);
		}
		for (i=0; (i==0)||(i<m->count); ++i) {
			dump_value (m->id & MMASK_TYPE, m, i, outfd);
			if ((i+1)<m->count) write (outfd, ",",1);
		}
		if (m->count > 0) {
			write (outfd, "]", 1);
		}
		m=m->next;
	}
	write (outfd,"\n",1);
}