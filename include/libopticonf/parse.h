#ifndef _PARSE_H
#define _PARSE_H 1

#include <libopticonf/var.h>

typedef enum parse_state_e {
    PSTATE_DICT_WAITKEY,
    PSTATE_DICT_KEY,
    PSTATE_DICT_KEY_QUOTED,
    PSTATE_DICT_WAITVALUE,
    PSTATE_DICT_VALUE,
    PSTATE_DICT_VALUE_QUOTED,
    PSTATE_ARRAY_WAITVALUE,
    PSTATE_ARRAY_VALUE,
    PSTATE_ARRAY_VALUE_QUOTED,
} parse_state;

int parse_config_level (var *, const char **, parse_state);
int parse_config (var *, const char *);

#endif
