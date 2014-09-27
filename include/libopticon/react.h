#ifndef _REACT_H
#define _REACT_H 1

#include <libopticon/var.h>

/* =============================== TYPES =============================== */

/** Enum to indicate what happened to the data when a reaction function
  * is called for it. */
typedef enum updatetype_s {
    UPDATE_NONE, /**< Nothing happened to the variable */
    UPDATE_ADD, /**< The variable is new since last time */
    UPDATE_CHANGE, /**< The variable has changed since last time */
    UPDATE_REMOVE /**< The variable no longer exists */
} updatetype;

typedef int (*reaction_f)(const char *, var *, updatetype);

/** Node in the match tree for configuration paths */
typedef struct pathnode_s {
    struct pathnode_s   *prev; /**< Link to neighbour */
    struct pathnode_s   *next; /**< Link to neighbour */
    struct pathnode_s   *parent; /**< Link to parent */
    struct pathnode_s   *first; /**< Link to children */
    struct pathnode_s   *last; /**< Link to children */
    char                 idmatch[128]; /**< Match string */
    int                  rcount; /**< Number of reaction functions */
    reaction_f          *reactions; /**< Reaction function array */
} pathnode;

/* ============================== GLOBALS ============================== */

extern pathnode OPTICONF_ROOT;

/* ============================= FUNCTIONS ============================= */

void opticonf_add_reaction (const char *, reaction_f);
void opticonf_handle_config (var *);

#endif
