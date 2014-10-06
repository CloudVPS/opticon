#ifndef _DUMP_H
#define _DUMP_H 1

#include <libopticon/var.h>
#include <libopticon/ioport.h>
#include <stdio.h>

int var_dump (var *, FILE *);
int var_write (var *, ioport *);
int var_write_indented (var *, ioport *, int);

#endif
