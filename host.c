#include <datatypes.h>
#include <util.h>

/** Allocate and initialize a host structure.
  * \return Pointer to the initialized struct. Caller is responsible for
  *         freeing allocated memory.
  */
host *host_alloc (void) {
	host *res = (host *) malloc (sizeof (host));
	res->prev = NULL;
	res->next = NULL;
	res->first = NULL;
	res->last = NULL;
	res->status = 0;
	return res;
}

/** Find or create a host structure for a specified tenant. 
  * \param tenantid The UUID for the tenant.
  * \param hostid The UUID for the host.
  */
host *host_find (uuid tenantid, uuid hostid) {
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

/** Allocate and initialize a meter structure. */
meter *meter_alloc (void) {
	meter *res = (meter *) malloc (sizeof (meter));
	res->next = NULL;
	res->prev = NULL;
	res->count = -1;
	res->d.u64 = NULL;
	return res;
}

/** Get (or create) a specific meter for a host.
  * \param host The host structure.
  * \param id The meterid (label and type).
  * \return Pointer to the meter structure, which will be
  *         linked into the host's meterlist.
  */
meter *host_get_meter (host *h, meterid_t id) {
	meterid_t rid = (id & (MMASK_TYPE | MMASK_NAME));
	meter *m = h->first;
	meter *nm = NULL;
	if (! m) {
		nm = meter_alloc();
		h->first = h->last = nm;
		nm->id = rid;
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
			nm->id = rid;
			return nm;
		}
	}
	
	return NULL;
}

/** Get a specific indexed integer value out of a meter */
uint32_t meter_get_uint (meter *m, unsigned int pos) {
	if (pos > m->count) return 0;
	if ((m->id & MMASK_TYPE) != MTYPE_INT) return 0;
	return m->d.u64[pos];
}

/** Get a specific indexed fractional value out of a meter */
double meter_get_frac (meter *m, unsigned int pos) {
	if (pos > m->count) return 0.0;
	if ((m->id & MMASK_TYPE) != MTYPE_INT) return 0.0;
	return m->d.frac[pos];
}

/** Get a string value out of a meter */
const char *meter_get_str (meter *m) {
	if ((m->id & MMASK_TYPE) != MTYPE_STR) return "";
	return m->d.str;
}

/** Fill up a meter with integer values */
meter *host_set_meter_uint (host *h, meterid_t id,
							unsigned int cnt,
							uint64_t *data) {
	
	unsigned int count = cnt ? cnt : 1;
	unsigned int i;
	meter *m = host_get_meter (h, id);
	m->count = cnt;
	
	if (m->d.any != NULL) {
		free (m->d.any);
		m->d.any = NULL;
	}
	
	m->d.u64 = (uint64_t *) malloc (count * sizeof (uint64_t));
	memcpy (m->d.u64, data, count * sizeof (uint64_t));
	return m;
}
