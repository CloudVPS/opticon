#include <sys/utsname.h>
#include <unistd.h>
#include <libopticon/var.h>
#include "probes.h"
#include "wordlist.h"

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

/** Get logged on users from unix who */
var *runprobe_who (probe *self) {
    char buf[1024];
    var *res = var_alloc();
    var *res_who = var_get_array_forkey (res, "who");
    char *c;
    wordlist *args;
    FILE *f = popen ("/usr/bin/who","r");
    if (! f) return res;
    while (! feof (f)) {
        *buf = 0;
        fgets (buf, 1023, f);
        buf[1023] = 0;
        if (*buf == 0) continue;
        args = wordlist_make (buf);
        if (args->argc > 4) {
            var *rec = var_add_dict (res_who);
            var_set_str_forkey (rec, "user", args->argv[0]);
            var_set_str_forkey (rec, "tty", args->argv[1]);
            if (args->argv[4][0] == '(') {
                strcpy (buf, args->argv[4]+1);
                c = strchr (buf, ')');
                if (c) *c = 0;
                var_set_str_forkey (rec, "remote", buf);
            }
            else var_set_str_forkey (rec, "remote", args->argv[4]);
        }
    }
    pclose (f);
    return res;
}

#define GLOBAL_BUILTINS \
    {"probe_hostname", runprobe_hostname}, \
    {"probe_uname", runprobe_uname}, \
    {"probe_who", runprobe_who}


#if defined(OS_DARWIN)
  #include "probes_darwin.c"
#elif defined (OS_LINUX)
  #include "probes_linux.c"
#else
  #error Undefined Operating System
#endif

