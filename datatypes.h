#ifndef _DATATYPES_H
#define _DATATYPES_H 1

typedef uint64_t		meterid_t;
typedef uint64_t		metertype_t;

#define MTYPE_INT 		0x0000000000000000
#define MTYPE_FRAC		0x4000000000000000
#define MTYPE_STR   	0x8000000000000000

#define MMASK_TYPE		0xc000000000000000
#define MMASK_COUNT		0x0000000000000007
#define MMASK_NAME		0x3ffffffffffffff8

meterid_t 	makeid (const char *label, metertype_t type, int pos);
void	  	id2label (meterid_t id, char *into);
metertype_t	id2type (meterid_t id);

typedef struct { uint64_t msb, uint64_t lsb } uuid_t;

typedef union {
	uint64_t		 *u64;
	double			 *frac;
	char			 *str;
} meterdata;

typedef struct {
	struct meter	*next;
	struct meter	*prev;

	meterid_t		 id;
	time_t			 lastmodified;
	uint32_t		 count;
	meterdata		 d;
} meter;

typedef struct {
	struct host		*next;
	struct host		*prev;
	uuid_t			 uuid;
	status_t		 status;
	meter			*first;
	meter			*last;
} host;	

typedef struct {
	struct tenant	*next;
	struct tenant	*prev;
	host			*first;
	host			*last;
	uuid_t			 uuid;
	char			*key;
} tenant;

typedef struct {
	tenant			*first;
	tenant			*last;
} tenantlist;

extern tenantlist TENANTS;

tenant *tenant_find (uuid_t tenantid);
void tenant_create (uuid_t tenantid, const char *key);

host *host_find (uuid_t tenantid, uuid_t hostid);

meter *host_get_meter (host *h, meterid_t id);
meter *host_set_meter_uint (host *h, meterid_t id, 
							unsigned int count,
							unsigned int *data);
meter *host_set_meter_frac (host *h, meterid_t id,
							unsigned int count,
							double *data);
meter *host_set_meter_str  (host *h, meter_id id, const char *str);

uint32_t	meter_get_uint (meter *, unsigned int pos);
double		meter_get_frac (meter *, unsigned int pos);
const char *meter_get_str (meter *, unsigned int pos);

#endif
