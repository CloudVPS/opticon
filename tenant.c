#include <datatypes.h>

tenantlist TENANTS;

void tenant_init (void) {
	TENANTS.first = TENANTS.last = NULL;
}

tenant *tenant_alloc (void) {
	tenant *res = (tenant *) malloc (sizeof(tenant));
	res->prev = res->next = NULL;
	res->key = NULL;
	return res;
}

tenant *tenant_find (uuid_t tenantid) {
	tenant *nt;
	tenant *t = TENANTS.first;
	if (! t) {
		nt = tenant_alloc();
		nt->uuid = tenantid;
		TENANTS.first = TENANTS.last = nt;
		return nt;
	}
	while (t) {
		if (t->uuid == tenantid) return t;
		if (t->next) {
			t = t->next;
		}
		else {
			nt = tenant_alloc;
			nt->uuid = tenantid;
			t->next = nt;
			nt->prev = t;
			TENANTS.last = nt;
			return nt;
		}
	}
}
