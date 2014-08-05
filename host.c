#include <datatypes.h>
#include <util.h>

host *host_alloc (void) {
	host *res = (host *) malloc (sizeof (host));
	res->prev = NULL;
	res->next = NULL;
	res->first = NULL;
	res->last = NULL;
	res->status = 0;
	return res;
}

host *host_find (uuid_t tenantid, uuid_t hostid) {
	host *h = NULL;
	host *nh = NULL;
	tenant *t = tenant_find (tenantid);
	if (! t) return NULL;
	
	h = t->first;
	if (! h) {
		h = host_alloc();
		h->uuid = hostid;
		t->first = t->last = h;
		return h;
	}
	
	while (h) {
		if (uuidcmp (h->uuid, hostid)) return h;
		if (h->next) h = h->next;
		else {
			nh = host_alloc();
			nh->uuid = hostid;
			h->next = nh;
			nh->prev = h;
			t->last = nh;
			return nh;
		}
	}
	
	return NULL;
}

meter *meter_alloc (void) {
	meter *res = (meter *) malloc (sizeof (meter));
	res->next = NULL;
	res->prev = NULL;
	res->count = -1;
	res->d.u64 = NULL;
	return res;
}

void *meter_allocarray (meter *m, int c) {
	int rc = c;
	uint64_t tp = m->id & MMASK_TYPE;
	if (rc==0) rc=1;
	size_t elmsz = 1;
	switch (tp) {
		case MTYPE_INT:
			elmsz = sizeof (uint64_t);
			break;
		
		case MTYPE_FRAC:
			elmsz = sizeof (double);
			break;
		
		default:
			elmsz = 256;
			break;
	}
	if (m->d.any == NULL) {
		return (void *) malloc (elmsz);
	}
	return (void *) realloc (m->d.any, elmsz);
}

void meter_setsize (meter *m, int c) {
	if (c == m->count) return;
	if (c == -1) {
		if (m->d.any != NULL) free (m->d.any);
		m->count = -1;
	}
	else if (c >= 0) {
		m->count = c;
		meter_allocarray (m, c);
	}
}

meter *host_get_meter (host *h, meterid_t id) {
	meterid_t rid = (id & (MMASK_TYPE | MMASK_NAME));
	meter *m = h->first;
	meter *nm = NULL;
	if (! m) {
		nm = meter_alloc();
		h->first = h->last = nm;
		return nm;
	}
	while (m) {
		if (m->id == rid) return m;
		if (m->next) m = m->next;
		else {
			nm = meter_alloc();
			m->next = nm;
			nm->prev = m;
			h->last = nm;
			return nm;
		}
	}
	
	return NULL;
}

uint32_t meter_get_uint (meter *m, unsigned int pos) {
	if (pos > m->count) return 0;
	if ((m->id & MMASK_TYPE) != MTYPE_INT) return 0;
	return m->d.u64[pos];
}

double meter_get_frac (meter *m, unsigned int pos) {
	if (pos > m->count) return 0.0;
	if ((m->id & MMASK_TYPE) != MTYPE_INT) return 0.0;
	return m->d.frac[pos];
}

const char *meter_get_str (meter *m) {
	if ((m->id & MMASK_TYPE) != MTYPE_STR) return "";
	return m->d.str;
}

