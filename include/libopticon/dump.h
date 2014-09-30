#ifndef _DUMP_H
#define _DUMP_H 1

#include <libopticon/var.h>
#include <libopticon/ioport.h>
#include <stdio.h>

int dump_var (var *, FILE *);
int write_var (var *, ioport *);

#endif
