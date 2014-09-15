#include <libopticonf/var.h>
#include <string.h>
#include <assert.h>

/** Allocate a var object */
var *var_alloc (void) {
    var *self = (var *) malloc (sizeof (var));
    if (! self) return NULL;
    
    self->next = self->prev = self->parent = self->root = NULL;
    self->id[0] = '\0';
    self->type = VAR_NULL;
    self->generation = 0;
    self->lastmodified = 0;
    self->firstseen = 0;
    return self;
}

/** Link a var into its parent (it is assumed to already have a unique
  * id.
  * \param self The fresh var object.
  * \param parent The parent var to add this node to.
  */
void var_link (var *self, var *parent) {
    if (! (self && parent)) return;
    
    self->parent = parent;
    self->root = (parent->root) ? parent->root : parent;
    self->next = NULL;

    /* copy generation data */    
    if (self->root) {
        self->firstseen = self->lastmodified = \
        self->generation = self->root->generation;
    }
    
    /* numbered array nodes have no id */
    if (self->id[0] == 0) {
        if (parent->type == VAR_NULL) {
            parent->type = VAR_ARRAY;
            parent->value.arr.first = NULL;
            parent->value.arr.last = NULL;
            parent->value.arr.count = 0;
            parent->value.arr.cachepos = -1;
            parent->value.arr.cachenode = NULL;
        }
        if (parent->type != VAR_ARRAY) return;
    }
    else {
        if (parent->type == VAR_NULL) {
            parent->type = VAR_DICT;
            parent->value.arr.first = NULL;
            parent->value.arr.last = NULL;
            parent->value.arr.count = 0;
        }
        if (parent->type != VAR_DICT) return;
    }
    
    if (parent->value.arr.last) {
        self->prev = parent->value.arr.last;
        parent->value.arr.last->next = self;
        parent->value.arr.last = self;
    }
    else {
        self->prev = NULL;
        parent->value.arr.first = parent->value.arr.last = self;
    }
    parent->value.arr.count++;
}

/** Unlink a var object from its parent structure. Deallocate all
  * memory associated with the var and any children.
  * \param self The var to free.
  */
void var_free (var *self) {
    if (self->parent) {
        if (self->prev) {
            if (self->next) {
                self->prev->next = self->next;
                self->next->prev = self->prev;
            }
            else {
                self->prev->next = NULL;
                self->parent->value.arr.last = self->prev;
            }
        }
        else {
            if (self->next) {
                self->next->prev = NULL;
                self->parent->value.arr.first = self->next;
            }
            else {
                self->parent->value.arr.first = NULL;
                self->parent->value.arr.last = NULL;
            }
        }
        self->parent->value.arr.count--;
        self->parent->value.arr.cachepos = -1;
    }
    
    if (self->type == VAR_STR) {
        free (self->value.sval);
    }
    
    if (self->type == VAR_ARRAY || self->type == VAR_DICT) {
        var *c = self->value.arr.first;
        var *nc;
        
        while (c) {
            nc = c->next;
            var_free (c);
            c = nc;
        }
    }

    free (self);    
}

/** Find a key in a dictionary node.
  * \param self The dictionary var
  * \param key The key to lookup
  * \return Pointer to the child var, or NULL if not found.
  */
var *var_find_key (var *self, const char *key) {
    if (self->type == VAR_NULL) {
        self->type = VAR_DICT;
        self->value.arr.first = self->value.arr.last = NULL;
        self->value.arr.count = 0;
        self->value.arr.cachepos = -1;
    }
    if (self->type != VAR_DICT) return NULL;
    var *c = self->value.arr.first;
    while (c) {
        if (strcmp (c->id, key) == 0) return c;
        c = c->next;
    }
    return NULL;
}

/** Find, or create, a dictionary node inside a dictionary var.
  * \param self The var to look into
  * \param key The key of the alleged/desired dict node.
  * \return var The dict node, or NULL if we ran into a conflict.
  */
var *var_get_dict_forkey (var *self, const char *key) {
    var *res = var_find_key (self, key);
    if (! res) {
        res = var_alloc();
        res->type = VAR_DICT;
        res->value.arr.first = res->value.arr.last = NULL;
        res->value.arr.count = 0;
        res->value.arr.cachepos = -1;
        strncpy (res->id, key, 127);
        res->id[127] = 0;
        var_link (res, self);
    }
    if (res->type == VAR_DICT) return res;
    return NULL;
}

/** Find, or create, an array node inside a dictionary var.
  * \param self The var to look into
  * \param key The key of the alleged/desired dict node.
  * \return var The dict node, or NULL if we ran into a conflict.
  */
var *var_get_array_forkey (var *self, const char *key) {
    var *res = var_find_key (self, key);
    if (! res) {
        res = var_alloc();
        res->type = VAR_ARRAY;
        res->value.arr.first = res->value.arr.last = NULL;
        res->value.arr.count = 0;
        res->value.arr.cachepos = -1;
        strncpy (res->id, key, 127);
        res->id[127] = 0;
        var_link (res, self);
    }
    if (res->type == VAR_ARRAY) return res;
    return NULL;
}

/** Get an integer value out of a dict var.
  * \param self The dict
  * \param key The key inside the dict
  * \return The integer value, or 0 if it couldn't be resolved. Strings
  *         will be autoconverted. PHP all the things.
  */
int var_get_int_forkey (var *self, const char *key) {
    var *res = var_find_key (self, key);
    if (! res) return 0;
    if (res->type == VAR_STR) {
        return atoi (res->value.sval);
    }
    if (res->type == VAR_INT) return res->value.ival;
    return 0;
}

/** Get a string value out of a dict var.
  * \param self The dict
  * \param key The key of the string inside the dict.
  * \return The string, or NULL if found incompatible/nonexistant. No ducktyping.
  */
const char *var_get_str_forkey (var *self, const char *key) {
    var *res = var_find_key (self, key);
    if (! res) return NULL;
    if (res->type != VAR_STR) return NULL;
    return res->value.sval;
}

/** Return the number of children of a keyed child-array or child-dict of
  * a dict variable.
  * \param self The parent dict
  * \param key The subkey
  * \return Number of members, or -1 for inappropriate type.
  */
int var_get_count (var *self) {
    if (self->type != VAR_ARRAY && self->type != VAR_DICT) return -1;
    return self->value.arr.count;    
}

/** Get the direct int value of a var object. */
int var_get_int (var *self) {
    if (self->type == VAR_STR) return atoi (self->value.sval);
    if (self->type == VAR_INT) return self->value.ival;
    return 0;
}

/** Get the direct string value of a var object. */
const char *var_get_str (var *self) {
    if (self->type == VAR_STR) return self->value.sval;
    return NULL;
}

/** Lookup a var inside a parent by its array index. Uses smart caching
  * to make sequential lookups lightweight.
  * \param self The array-object
  * \param index The index within the array, -1 and down for accessing
                 from the last item down.
  * \return The variable, or NULL if it couldn't be found.
  */
var *var_find_index (var *self, int index) {
    if (self->type != VAR_ARRAY && self->type != VAR_DICT) return NULL;
    var *res = NULL;
    int cindex = (index < 0) ? self->value.arr.count - index : index;
    if (cindex < 0) return NULL;
    if (cindex >= self->value.arr.count) return NULL;
    
    int cpos = self->value.arr.cachepos;
    if (cpos == cindex) return self->value.arr.cachenode;

    if (index == -1) res = self->value.arr.last;    
    if (cpos == cindex+1) res = self->value.arr.cachenode->prev;
    if (cindex && cpos == cindex-1) res = self->value.arr.cachenode->next;
    
    if (! res) {
        res = self->value.arr.first;
        for (int i=0; res && (i<cpos); ++i) {
            res = res->next;
        }
    }
    
    if (! res) return NULL;
    self->value.arr.cachepos = index;
    self->value.arr.cachenode = res;
    return res;
}

/** Lookup a dict var inside an array var.
  * \param self The array to look inside
  * \param idx The index (negative for measuring from the end).
  * 'return The dict node, or NULL if it could not be arranged.
  */
var *var_get_dict_atindex (var *self, int idx) {
    var *res = var_find_index (self, idx);
    if (! res) return NULL;
    
    if (res->type == VAR_NULL) {
        res->type = VAR_DICT;
        res->value.arr.first = res->value.arr.last = NULL;
        res->value.arr.count = 0;
        res->value.arr.cachepos = -1;
        res->id[0] = 0;
        var_link (res, self);
    }
    if (res->type != VAR_DICT) return NULL;
    return res;
}

/** Lookup an array var inside an array var.
  * \param self The array to look inside
  * \param idx The index (negative for measuring from the end).
  * 'return The dict node, or NULL if it could not be arranged.
  */
var *var_get_array_atindex (var *self, int idx) {
    var *res = var_find_index (self, idx);
    if (! res) return NULL;
    
    if (res->type == VAR_NULL) {
        res->type = VAR_ARRAY;
        res->value.arr.first = res->value.arr.last = NULL;
        res->value.arr.count = 0;
        res->value.arr.cachepos = -1;
        res->id[0] = 0;
        var_link (res, self);
    }
    if (res->type != VAR_ARRAY) return NULL;
    return res;
}

/** Get the int value of a var inside an array var.
  * \param self The array
  * \param idx The array index (negative for measuring from the end).
  * \return The integer value, failed lookups will yield 0.
  */
int var_get_int_atindex (var *self, int idx) {
    var *res = var_find_index (self, idx);
    if (! res) return 0;
    if (res->type == VAR_INT) return res->value.ival;
    if (res->type == VAR_STR) return atoi (res->value.sval);
    return 0;
}

/** Get the string value of a var inside an array var.
  * \param self The array
  * \param idx The array index (negative for measuring from the end).
  * \return The string value, or NULL.
  */
const char *var_get_str_atindex (var *self, int idx) {
    var *res = var_find_index (self, idx);
    if (! res) return 0;
    if (res->type != VAR_STR) return NULL;
    return res->value.sval;
}

/** Increase the generation counter of a variable space. When a new versin
  * of the same structured document is loaded again, values that are set
  * will have their generation updated to the latest number. Values that
  * actually -change- in the process will also see their lastmodified
  * updated. Reflective code can use this information later to figure out
  * what changed about a particular configuration.
  */
void var_new_generation (var *rootnode) {
    assert (rootnode->root == NULL);
    rootnode->generation++;
}

/** When everything interesting that can be done with the generational
  * informational after a new round of loading has been done, we are potentially
  * left with zombie var-nodes that didn't get refreshed. This function will
  * reap those stale nodes out of a variable space recursively.
  */
void var_clean_generation (var *node) {
    uint32_t needgen = (node->root) ? node->root->generation : node->generation;
    if (node->type != VAR_DICT && node->type != VAR_ARRAY) return;
    var *crsr = node->value.arr.first;
    var *ncrsr;
    while (crsr) {
        ncrsr = crsr->next;
        /* time for a-reapin'? */
        if (crsr->generation < needgen) {
            var_free (crsr);
        }
        else var_clean_generation (crsr);
        crsr = ncrsr;
    }
}

/** Get a hold of a keyed sub-var of a dictionary var, whether it exists
  * or not. Newly created nodes will initialize to type VAR_NULL.
  * \param self The parent dict.
  * \param key The key to look up.
  * \return The var found or created.
  */
var *var_get_or_make (var *self, const char *key, vartype tp) {
    var *res = var_find_key (self, key);
    if (! res) {
        res = var_alloc();
        strncpy (res->id, key, 127);
        res->id[127] = 0;
        var_link (res, self);
    }
    return res;
}

/** Update the generational data for a var. Will traverse up the tree in order
  * to get the generation and modified data set.
  * \param v The loaded object.
  * \param is_updated 1 if lastmodified should be changed.
  */
void var_update_gendata (var *v, int is_updated) {
    while (v) {
        if (! v->root) break;
        if (is_updated) {
            if (v->lastmodified == v->root->generation) break;
            v->lastmodified = v->root->generation;
        }
        if (v->generation == v->root->generation) break;
        v->generation = v->root->generation;
        v = v->parent;
    }
}

/** Set the integer value of a dict-var sub-var.
  * \param self The dict.
  * \param key The key within the dict.
  * \param val The value to set.
  */
void var_set_int_forkey (var *self, const char *key, int val) {
    var *v = var_get_or_make (self, key, VAR_INT);
    if (! v) return;
    var_set_int (v, val);
}

/** Set the direct integer value of a var */
void var_set_int (var *v, int val) {
    int is_orig = 0;
    
    v->generation = v->root->generation;
    if (v->type == VAR_NULL) {
        v->type = VAR_INT;
        v->lastmodified = v->generation;
        is_orig = 1;
    }
    
    if (v->type != VAR_INT) return;
    if (!is_orig && (v->value.ival != val)) {
        is_orig = 1;
    }
    v->value.ival = val;
    var_update_gendata (v, is_orig);
}

/** Set the string value of a dict-var sub-var.
  * \param self The dict.
  * \param key The key within the dict.
  * \param val The value to set.
  */
void var_set_str_forkey (var *self, const char *key, const char *val) {
    var *v = var_get_or_make (self, key, VAR_STR);
    if (! v) return;
    var_set_str (v, val);
}

/** Set the direct string value of a var */
void var_set_str (var *v, const char *val) {
    int is_orig = 0;
    
    if (v->type == VAR_NULL) {
        v->type = VAR_STR;
        v->value.sval = strdup (val);
        var_update_gendata (v, 1);
    }
    else if (v->type == VAR_STR) {
        if (strcmp (v->value.sval, val) == 0) {
            var_update_gendata (v, 0);
        }
        else {
            free (v->value.sval);
            v->value.sval = strdup (val);
            var_update_gendata (v, 1);
        }
    }
}

/** Clear an array. Arrays are always reloaded in bulk (with all children
  * showing up with the last generation as the firstseen and lastmodified).
  * \param v The array to clear.
  */
void var_clear_array (var *v) {
    if (v->type != VAR_ARRAY) return;
    var *c, *nc;
    c = v->value.arr.first;
    while (c) {
        nc = c->next;
        var_free (c);
        c = nc;
    }
    
    var_update_gendata (v, 1);
}

/** Add an integer value to an array var.
  * \param self The array
  * \param nval The integer to add.
  */
void var_add_int (var *self, int nval) {
    if (self->type != VAR_ARRAY) return;
    var *nvar = var_alloc();
    nvar->type = VAR_INT;
    nvar->id[0] = 0;
    nvar->value.ival = nval;
    var_link (nvar, self);
}

/** Add a string value to an array var.
  * \param self The array
  * \param nval The string to add (will be copied).
  */
void var_add_str (var *self, const char *nval) {
    if (self->type != VAR_ARRAY) return;
    var *nvar = var_alloc();
    nvar->type = VAR_STR;
    nvar->id[0] = 0;
    nvar->value.sval = strdup (nval);
    var_link (nvar, self);
}
