#ifndef _DEFAULTS_H
#define _DEFAULTS_H 1

#ifdef _DEFAULTS_C
  #define parameter
  #define defaultvalue(foo) = foo
#else
  #define parameter extern
  #define defaultvalue(foo)
#endif

parameter int         default_meter_timeout     defaultvalue(600);
parameter const char *default_service_user      defaultvalue("opticon");

#endif
