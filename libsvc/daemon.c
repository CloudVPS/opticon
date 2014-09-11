#include <daemon.h>

/** Fork the process into the background, with a watchdog guarding its
  * execution. If the --foreground flag is provided in argv[1], no 
  * daemonization will take place, and execution will be switched to
  * the main function directly. Signals intended for the service
  * process should be sent to the watchdog, which will forward the
  * signal (and recuse itself from respawning in case of SIGTERM).
  * \param pidfilepath Path and filename of the pidfile to save.
  * \param argc The libc argc
  * \param argv The libc argv
  * \param call The main function to run the service.
  * \return 0 on failure, 1 on success (caller should exit).
  */
int daemonize (const char *pidfilepath, int argc,
               const char *argv[], main_f call) {
    pid_t pwatchdog;
    pid_t pservice;
    uid_t svcuid;
    gid_t svcgid;
    struct passwd *pwd = NULL;
    FILE *pidfile;
    char pidbuf[128];
    
    if ((argc>1) && (strcmp (argv[1], "--foreground") == 0)) {
        (void) main_f (argc, argv);
        return 1;
    }
    
    /* Make sure we're not already running */
    pidfile = fopen (pidfilepath, "r");
    if (pidfile != NULL) {
        fread (pidbuf, 128, 1, pidfile);
        fclose (pidfile);
        pidbuf[127] = 0;
        pwatchdog = aoti (pidbuf);
        if (pwatchdog && (kill (pwatchdog, 0) == 0)) {
            fprintf (stderr, "%%ERR%% Already running\n");
            return 0;
        }
    }
    
    /* If we're root, change. */
    if (geteuid() == 0) {
        pwd = getpwnam (default_service_user);
        if (! pwd) {
            fprintf (stderr, "%%ERR%% User %s not found\n",
                     default_service_user);
            return 0;
        }
    
        svcuid = pwd->pw_uid;
        svcgid = pwd->pw_gid;
    
        /* Create the pidfile so we can chown it */
        pidfile = fopen (pidfilepath, "w");
        fclose (pidfile);
        chown (pidfilepath, svcuid, svcgid);
    
        /* Drop root privileges */
        setregid (svcgid, svcgid);
        setreuid (svcuid, svcuid);
    }
    
    switch (pwatchdog = fork()) {
        case -1:
            fprintf (stderr, "%%ERR%% Fork failed\n");
            return 0;
        
        case 0:
            watchdog_main (pidfilepath, argc, argv, call);
            exit (0);
            break;
        
        default:
            break;
    }
    
    return 1;
}
