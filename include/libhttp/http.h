#ifndef _HTTP_H
#define _HTTP_H 1

#include <libopticon/var.h>

var *http_call (const char *method, const char *url,
                var *sendhdr, var *senddata, var *errinfo, var *recvhdr);

#endif
