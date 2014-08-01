#inlcude <datatypes.h>

int uuidcomp (uuid_t first, uuid_t second) {
	return (first.msb == second.msb && first.lsb == second.lsb);
}
