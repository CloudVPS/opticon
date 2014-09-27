#include <sys/utsname.h>
#include <unistd.h>
#include <libopticon/var.h>
#include "probes.h"

/** Get the hostname. POSIX compliant */
var *runprobe_hostname (probe *self) {
    char out[256];
    gethostname (out, 255);
    var *res = var_alloc();
    var_set_str_forkey (res, "hostname", out);
    return res;
}

/** Get kernel version information. POSIX compliant. */
var *runprobe_uname (probe *self) {
    var *res = var_alloc();
    var *v_os = var_get_dict_forkey (res, "os");
    struct utsname uts;
    uname (&uts);
    var_set_str_forkey (v_os, "kernel", uts.sysname);
    var_set_str_forkey (v_os, "version", uts.release);
    var_set_str_forkey (v_os, "arch", uts.machine);
    return res;
}

#define GLOBAL_BUILTINS \
    {"probe_hostname", runprobe_hostname}, \
    {"probe_uname", runprobe_uname}


#if defined(OS_DARWIN)
  #include "probes_darwin.c"
#elif defined (OS_LINUX)
  #include "probes_linux.c"
#else
  #error Undefined Operating System
#endif

