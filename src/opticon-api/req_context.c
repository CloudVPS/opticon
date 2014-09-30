#include <stdlib.h>
#include <unistd.h>
#include <libopticon/util.h>
#include <libopticon/parse.h>
#include "req_context.h"

req_context *req_context_alloc (void) {
    req_context *self = (req_context *) malloc (sizeof (req_context));
    if (self) {
        self->headers = var_alloc();
        self->bodyjson = var_alloc();
        self->response = var_alloc();
        self->status = 500;
        self->url = NULL;
        self->cmd = NULL;
        self->body = NULL;
        self->bodysz = self->bodyalloc = 0;
        memset (&self->tenantid, 0, sizeof (uuid));
        memset (&self->hostid, 0, sizeof (uuid));
        memset (&self->opticon_token, 0, sizeof (uuid));
        self->openstack_token = NULL;
    }
    return self;
}

void req_context_free (req_context *self) {
    var_free (self->headers);
    var_free (self->bodyjson);
    var_free (self->response);
    if (self->url) {
        free (self->url);
        if (self->cmd) free (self->cmd);
    }
    if (self->body) free (self->body);
    if (self->openstack_token) free (self->openstack_token);
    free (self);
}

void req_context_set_header (req_context *self, const char *hdrname,
                             const char *hdrval) {
    char *canon = strdup (hdrname);
    char *c = canon;
    int needcap=1;
    while (*c) {
        if (needcap) { *c = toupper(*c); needcap = 0; }
        else if (*c == '-') needcap = 1;
        else *c = tolower (*c);
        c++;
    }
    var_set_str_forkey (self->headers, canon, hdrval);
    
    /** Handle Openstack Token */
    if (strcmp (canon, "X-Auth-Token") == 0) {
        self->openstack_token = strdup (hdrval);
    }
    else if (strcmp (canon, "X-Opticon-Token") == 0) {
        self->opticon_token = mkuuid (hdrval);
    }
    
    free (canon);
}

int req_context_parse_body (req_context *self) {
    if (! self->body) return 0;
    self->body[self->bodysz] = 0;
    return parse_json (self->bodyjson, self->body);
}

static size_t mkallocsz (size_t target) {
    if (target < 4096) return 4096;
    if (target < 8192) return 8192;
    if (target < 16384) return 16384;
    if (target < 65536) return 65536;
    return target + 4096;
}

void req_context_append_body (req_context *self, const char *dt, size_t sz) {
    if (! self->body) {
        self->bodyalloc = mkallocsz (sz);
        self->body = (char *) malloc (self->bodyalloc);
        self->bodysz = 0;
    }
    if ((self->bodysz + sz + 1) > self->bodyalloc) {
        self->bodyalloc = mkallocsz (self->bodysz + sz + 1);
        self->body = (char *) realloc (self->body, self->bodyalloc);
    }
    memcpy (self->body + self->bodysz, dt, sz);
    self->bodysz += sz;
    self->body[self->bodysz] = 0;
}

void req_context_set_url (req_context *self, const char *url) {
    if (self->url) free (self->url);
    self->url = strdup (url);
    char *splitstr = strdup (url);
    char *tofree = splitstr;
    char *token;
    int logicalpos = 0;
    
    while ((token = strsep (&splitstr, "/")) != NULL) {
        switch (logicalpos) {
            case 0:
                /* skip any initial slash */
                if (! strlen (token)) continue;
                if (strcmp (token, "v1.0") == 0) continue;
                if (strlen (token) == 36) {
                    self->tenantid = mkuuid (token);
                    if (self->tenantid.lsb && self->tenantid.msb) {
                        logicalpos++;
                    }
                }
                break;
            
            case 1:
                 if (strlen (token) == 36) {
                    self->hostid = mkuuid (token);
                 }
                 else {
                    if (self->cmd) free (self->cmd);
                    self->cmd = strdup (token);
                    logicalpos++;
                 }
                 break;
            
            default:
                break;
        }
    }
    free (tofree);
}
