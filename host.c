#include <datatypes.h>

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
		h = host_alloc;
		h->uuid = hostid;
		t->first = t->last = h;
		return h;
	}
	
	while (h) {
		if (h->uuid == hostid) return h;
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
}

meter *meter_alloc (void) {
	meter *res = (meter *) malloc (sizeof (meter));
	res->next = NULL;
	res->prev = NULL;
	res->count = -1;
	res->d.u64 = NULL;
}

meter *host_get_meter (host *h, meterid_t id) {
	meterid_t rid = (id & (MMASK_TYPE | MMASK_NAME));
	meter *m = h->first;
	neter *nm = NULL;
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
}
