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
