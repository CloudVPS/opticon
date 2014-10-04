#ifndef _PRETTYPRINT_H
#define _PRETTYPRINT_H 1

#include <libopticon/var.h>

/** Table alignment indicator */
typedef enum {
    CA_NULL,
    CA_L,
    CA_R
} columnalign;

void print_hdr (const char *);
void print_value (const char *, const char *, ...);
void print_array (const char *key, var *arr);
void print_values (var *apires, const char *pfx, var *mdef);
void print_table (var *arr, const char **hdr, const char **fld,
                  columnalign *align, vartype *typ, int *wid,
                  const char **suffx, int *div);
void print_generic_table (var *);
void print_tables (var *);

#endif
