#ifndef _REACT_H
#define _REACT_H 1

#include <libopticonf/var.h>

/* =============================== TYPES =============================== */

typedef enum updatetype_s {
    UPDATE_NONE,
    UPDATE_ADD,
    UPDATE_CHANGE,
    UPDATE_REMOVE
} updatetype;

typedef int (*reaction_f)(const char *, var *, updatetype);

typedef struct pathnode_s {
    struct pathnode_s   *prev;
    struct pathnode_s   *next;
    struct pathnode_s   *parent;
    struct pathnode_s   *first;
    struct pathnode_s   *last;
    char                 idmatch[128];
    int                  rcount;
    reaction_f          *reactions;
} pathnode;

/* ============================== GLOBALS ============================== */

extern pathnode OPTICONF_ROOT;

/* ============================= FUNCTIONS ============================= */

void opticonf_add_reaction (const char *, reaction_f);
void opticonf_handle_config (var *);

#endif
