#ifndef _IOPORT_FILE_H
#define _IOPORT_FILE_H 1

#include <libopticon/ioport.h>

/* ============================= FUNCTIONS ============================= */

ioport      *ioport_create_filewriter (FILE *);
ioport      *ioport_create_dualfilewriter (FILE *, FILE *);
ioport      *ioport_create_filereader (FILE *);

#endif
