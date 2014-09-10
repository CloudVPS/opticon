#ifndef _DEFAULTS_H
#define _DEFAULTS_H 1

#ifdef _DEFAULTS_C
  #define parameter
  #define defaultvalue(foo) = foo
#else
  #define parameter extern
  #define defaultvalue(foo)
#endif

parameter int default_meter_timeout defaultvalue(300);

#endif
