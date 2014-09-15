#ifndef _VAR_H
#define _VAR_H 1

typedef struct varlink_s {
    struct var_s    *first;
    struct var_s    *last;
    uint32_t         count;
    int              cachepos;
    struct var_s    *cachenode;
} varlink;

enum vartype {
    VAR_NULL,
    VAR_INT,
    VAR_STR,
    VAR_DICT,
    VAR_ARRAY
}

typedef union varvalue_s {
    char            *sval;
    int              ival;
    varlink          arr;
} varvalue;

typedef struct var_s {
    struct var_s    *next;
    struct var_s    *prev;
    struct var_s    *parent;
    struct var_s    *root;
    char             id[128];
    vartype          type;
    varvalue         value;
    uint32_t         generation;
    uint32_t         lastmodified;
} var;

var         *var_alloc (void);
void         var_link (var *, var *parent);
void         var_free (var *);

var         *var_get_dict (var *, const char *);
var         *var_get_array (var *, const char *);
int          var_get_int (var *, const char *);
const char  *var_get_str (var *, const char *);
var         *var_get_dict_atindex (var *, int);
var         *var_get_array_atindex (var *, int);
int          var_get_int_atindex (var *, int);
const char  *var_get_str_atindex (var *, int);

void         var_new_generation (var *);
void         var_clean_generation (var *);
void         var_set_int (var *, const char *, int);
void         var_set_str (var *, const char *, const char *);
void         var_set_int_atindex (var *, int, int);
void         var_set_str_atindex (var *, int, const char *);

#endif
