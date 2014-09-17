#include <libsvc/cliopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const char **cliopt_dispatch (cliopt *opt, const char **argv, int *argc) {
    int inargc = *argc;
    *argc = 0;
    const char **res = (const char **) malloc (inargc * sizeof (void*));
    const char *cur;
    const char *val;
    const char *arg;
    int optfound;
    int keepparsing = 1;
    int o;
    
    for (int i=0; i<inargc; ++i) {
        cur = argv[i];
        if (keepparsing && cur[0] == '-') {
            if (cur[1] == '-' && cur[2] == 0) {
                keepparsing = 0;
                continue;
            }
            optfound = 0;
            for (o=0; opt[o].longarg; ++o) {
                arg = (cur[1]=='-') ? opt[o].longarg : opt[o].shortarg;
                if (strcmp (arg, cur) != 0) continue;
                optfound = 1;
                val = NULL;
                if (opt[o].flag & OPT_VALUE) {
                    if ((i+1)>=inargc) {
                        fprintf (stderr, "%% Missing argument for "
                                 "%s\n", cur);
                        free (res);
                        return NULL;
                    }
                    val = argv[i+1];
                    ++i;
                }
                opt[o].handler (cur, val);
                opt[o].flag |= OPT_ISSET;
                break;
            }
            if (! optfound) {
                fprintf (stderr, "%% Invalid option: %s\n", cur);
                free (res);
                return NULL;
            }
        }
        else {
            res[*argc] = argv[i];
            (*argc)++;
        }
    }
    
    for (o=0; opt[o].longarg; ++o) {
        if (opt[o].flag & OPT_VALUE) {
            if (! (opt[o].flag & OPT_ISSET)) {
                if (opt[o].defaultvalue) {
                    opt[o].handler (opt[o].longarg, opt[o].defaultvalue);
                }
                else {
                    fprintf (stderr, "%% Missing required flag: %s\n",
                             opt[o].longarg);
                    free (res);
                    return NULL;
                }
            }
        }
    }
    
    return res;
}
