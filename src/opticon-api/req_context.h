#ifndef _REQ_CONTEXT_H
#define _REQ_CONTEXT_H 1

#include <libopticon/var.h>

typedef enum {
    REQ_GET,
    REQ_POST,
    REQ_PUT,
    REQ_DELETE,
    REQ_OTHER
} req_method;

typedef enum {
    AUTH_GUEST,
    AUTH_USER,
    AUTH_ADMIN
} auth_level;

typedef struct req_context_s {
    var         *headers; /**< HTTP headers */
    var         *bodyjson; /**< Parsed JSON */
    var         *response; /**< Prepared JSON response */
    int          status; /**< HTTP response code */
    req_method   method; /**< HTTP request method */
    char        *url; /**< Requested URL */
    char        *cmd; /**< Command part of URL */
    char        *arg; /**< Optional argument to cmd */
    char        *ctype; /**< Content type */
    char        *body; /**< Received body data */
    size_t       bodysz; /**< Size of body data */
    size_t       bodyalloc; /**< Allocated size of body buffer */
    uuid         tenantid; /**< URL-parsed tenantid, if any */
    uuid         hostid; /**< URL-parsed hostid, if any */
    uuid         opticon_token; /**< Header-parsed opticon token */
    char        *openstack_token; /**< Header-pasred openstack token */
    var         *auth_data; /**< Authentication data */
} req_context;

req_context *req_context_alloc (void);
void         req_context_free (void);
void         req_context_set_header (req_context *self, const char *hdr,
                                     const char *val);
int          req_context_parse_body (req_context *self);
void         req_context_append_body (req_context *, const char *, size_t);
void         req_context_set_url (req_context *, const char *);
void         req_context_set_method (req_context *, const char *);

#endif
