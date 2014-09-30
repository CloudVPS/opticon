#ifndef _REQ_CONTEXT_H
#define _REQ_CONTEXT_H 1

#include <libopticon/var.h>
#include <microhttpd.h>

typedef uint8_t req_method;
#define REQ_GET 0x01
#define REQ_POST 0x02
#define REQ_PUT 0x04
#define REQ_DELETE 0x08
#define REQ_OTHER 0x10
#define REQ_UPDATE (REQ_POST|REQ_PUT)
#define REQ_ANY (REQ_UPDATE|REQ_GET|REQ_OTHER)

typedef enum {
    AUTH_GUEST,
    AUTH_USER,
    AUTH_ADMIN
} auth_level;

/** Request context for HTTP handling */
typedef struct req_context_s {
    var         *headers; /**< HTTP headers */
    var         *bodyjson; /**< Parsed JSON */
    var         *response; /**< Prepared JSON response */
    int          status; /**< HTTP response code */
    req_method   method; /**< HTTP request method */
    char        *url; /**< Requested URL */
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

/** Arguments matched out of a URL */
typedef struct req_arg_s {
    int argc; /**< number of allocated elements */
    char **argv; /**< Element array (owned) */
} req_arg;

/** Handler function for a specified URL path */
typedef int (*path_f)(req_context *, req_arg *, var *resp, int *st);

/** Path match definition*/
typedef struct req_match_s {
    struct req_match_s  *next; /**< Link neighbour */
    struct req_match_s  *prev; /**< Link neighbour */
    const char          *matchstr; /**< Match definition. */
    req_method           method_mask; /**< Methods affected */
    path_f               func; /**< Handler function */
} req_match;

/** List header */
typedef struct req_matchlist_s {
    req_match   *first;
    req_match   *last;
} req_matchlist;

extern req_matchlist REQ_MATCHES;

int          err_server_error (req_context *, req_arg *, var *, int *);

req_arg     *req_arg_alloc (void);
void         req_arg_clear (req_arg *);
void         req_arg_free (req_arg *);
void         req_arg_add (req_arg *, const char *);

void         req_matchlist_init (req_matchlist *);
void         req_matchlist_add (req_matchlist *, const char *,
                                req_method, path_f);
void         req_matchlist_dispatch (req_matchlist *, const char *url,
                                     req_context *ctx,
                                     struct MHD_Connection *conn);

req_context *req_context_alloc (void);
void         req_context_free (req_context *);
void         req_context_set_header (req_context *self, const char *hdr,
                                     const char *val);
int          req_context_parse_body (req_context *self);
void         req_context_append_body (req_context *, const char *, size_t);
void         req_context_set_url (req_context *, const char *);
void         req_context_set_method (req_context *, const char *);

#endif
