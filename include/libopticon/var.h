#ifndef _VAR_H
#define _VAR_H 1

#include <sys/types.h>
#include <stdlib.h>
#include <libopticon/uuid.h>
#include <inttypes.h>

/* =============================== TYPES =============================== */

struct var_s; /* forward declaration */

/** Lookup structure for var dictionaries and arrays */
typedef struct varlink_s {
    struct var_s    *first; /**< First child */
    struct var_s    *last; /**< Last child */
    uint32_t         count; /**< Number of children */
    int              cachepos; /**< Position of last lookup (-1 = none) */
    struct var_s    *cachenode; /**< Node of last lookup */
} varlink;

/** Supported variable types for var */
typedef enum vartype_e {
    VAR_NULL, /**< Unset value */
    VAR_INT, /**< Integer */
    VAR_DOUBLE, /**< Floating point */
    VAR_STR, /**< Allocated string */
    VAR_DICT, /**< Dictionary */
    VAR_ARRAY /**< Awkward numbered array */
} vartype;

/** Value union for var */
typedef union varvalue_s {
    char            *sval; /**< String value (var owns memory) */
    uint64_t         ival; /**< Integer value */
    double           dval; /**< Floating point value */
    varlink          arr; /**< Dictionary or array value */
} varvalue;

/** Generic variable storage */
typedef struct var_s {
    struct var_s    *prev; /**< Link to previous node */
    struct var_s    *next; /**< Link to next node */
    struct var_s    *parent; /**< Link to parent node */
    struct var_s    *root; /**< Link to root of variable space */
    char             id[128]; /**< Key, or "" for numbered item */
    uint32_t         hashcode; /**< hash of key, or 0 */
    vartype          type; /**< Value type */
    varvalue         value; /**< Value */
    uint32_t         generation; /**< Last generation var was seen */
    uint32_t         lastmodified; /**< Last generation var was modified */
    uint32_t         firstseen; /**< First generation var was seen */
} var;

/* ============================= FUNCTIONS ============================= */

var         *var_alloc (void);
void         var_link (var *, var *parent);
void         var_free (var *);
void         var_copy (var *into, var *orig);

var         *var_find_key (var *, const char *);
var         *var_find_index (var *, int);
int          var_get_count (var *);
uint64_t     var_get_int (var *);
double       var_get_double (var *);
const char  *var_get_str (var *);
uuid         var_get_uuid (var *);
time_t       var_get_time (var *);
var         *var_get_dict_forkey (var *, const char *);
var         *var_get_array_forkey (var *, const char *);
uint64_t     var_get_int_forkey (var *, const char *);
double       var_get_double_forkey (var *, const char *);
const char  *var_get_str_forkey (var *, const char *);
uuid         var_get_uuid_forkey (var *, const char *);
time_t       var_get_time_forkey (var *, const char *);
var         *var_get_dict_atindex (var *, int);
var         *var_get_array_atindex (var *, int);
uint64_t     var_get_int_atindex (var *, int);
double       var_get_double_atindex (var *, int);
const char  *var_get_str_atindex (var *, int);
uuid         var_get_uuid_atindex (var *, int);
time_t       var_get_time_atindex (var *, int);

void         var_new_generation (var *);
void         var_clean_generation (var *);
void         var_set_int (var *, uint64_t);
void         var_set_double (var *, double);
void         var_set_str (var *, const char *);
void         var_own_str (var *, char *);
void         var_set_uuid (var *, uuid);
void         var_set_time (var *, time_t);
void         var_set_unixtime (var *, time_t);
void         var_set_int_forkey (var *, const char *, uint64_t);
void         var_set_double_forkey (var *, const char *, double);
void         var_set_str_forkey (var *, const char *, const char *);
void         var_set_uuid_forkey (var *, const char *, uuid);
void         var_set_time_forkey (var *, const char *, time_t);
void         var_set_unixtime_forkey (var *, const char *, time_t);
void         var_delete_key (var *, const char *);
void         var_clear_array (var *);
void         var_add_double (var *, double);
void         var_add_int (var *, uint64_t);
void         var_add_str (var *, const char *);
void         var_add_uuid (var *, uuid);
void         var_add_time (var *, time_t);
void         var_add_unixtime (var *, time_t);
var         *var_add_array (var *);
var         *var_add_dict (var *);

#endif
