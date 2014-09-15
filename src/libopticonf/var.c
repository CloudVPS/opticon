#include <libopticonf/var.h>

/** Allocate a var object */
var *var_alloc (void) {
    var *self = (var *) malloc (sizeof (var));
    if (! self) return res;
    
    self->next = self->prev = self->parent = self->root = NULL;
    self->id[0] = '\0';
    self->type = VAR_NULL;
    self->generation = 0;
    self->lastmodified = 0;
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
        self->lastmodified = self->generation = self->root->generation;
    }
    
    /* numbered array nodes have no id */
    if (self->id[0]) {
        if (parent->type == VAR_NULL) {
            parent->type = VAR_ARRAY;
            parent->value.arr.first = NULL
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
            parent->value.arr_first = NULL;
            parent->value.arr_last = NULL;
            parent->value.arr.count = 0;
        }
        if (parent->type != VAR_DICT) return;
    }
    
    if (parent->value.arr.last) {
        self->prev = parent->value.arr.last;
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
        var *c = self->varlue.arr.first;
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
var *var_get_dict (var *self, const char *key) {
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
}

