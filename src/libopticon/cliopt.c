#include <libopticon/cliopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/** Pass through the application argc and argv, handling any command
  * line flags through a cliopt matchlist. Returns NULL and prints
  * an error if an undefined option is encountered.
  * \param opt The list of option definitions.
  * \param argv The original argv (will remain untouched).
  * \param argc The original argc (will be overwritten with the new count).
  * \return A newly allocated argv array with all matched flags removed.
  */
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

/** Run a sub-command (probably provided on the command line). Will
  * print an error if there is no match.
  * \param cmds The definition of sub-commands.
  * \param command The given command.
  * \param argc Application argc.
  * \param argv Application argv.
  * \return The program return code.
  */
int cliopt_runcommand (clicmd *cmds, const char *command,
                       int argc, const char **argv) {
    for (int o=0; cmds[o].command; ++o) {
        if (strcmp (cmds[o].command, command) == 0) {
            return cmds[o].func (argc, argv);
        }
    }
    
    fprintf (stderr, "%% Unknown command: %s\n", command);
    return 1;
}
