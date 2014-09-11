#include <libopticon/defaults.h>
#include <libsvc/daemon.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

static int WATCHDOG_EXIT; /**< Flag for exiting the respawn-loop */
static pid_t SERVICE_PID; /**< Currently active pid for the service */

/** Signal handler for the watchdog. Forwards bound signals. If a SIGTERM
  * is passed, the WATCHDOG_EXIT flag is flipped so that we stop
  * respawning.
  * \param sig The signal to deliver.
  */
void watchdog_sighandler (int sig) {
    if (sig == SIGTERM) WATCHDOG_EXIT=1;
    kill (SERVICE_PID, sig);
}

/** Implementation of the watchdog process, responsible for spawning,
  * and respawning, of the actual service process, and for forwarding
  * relevant signals sent to its pid (which is what ends up in the
  * pidfile).
  * \param argc The libc argc
  * \param argv The libc argv
  * \param call The main function to run the service.
  */
void watchdog_main (int argc, const char *argv[], main_f call) {
    WATCHDOG_EXIT = 0;
    SERVICE_PID = 0;
    pid_t pid;
    int retval;
    
    signal (SIGTERM, watchdog_sighandler);
    signal (SIGHUP, watchdog_sighandler);
    signal (SIGUSR1, watchdog_sighandler);
    
    do {
        switch (pid = fork()) {
            case -1:
                sleep (60); /* nothing sane to do here */
                continue;
            
            case 0:
                call (argc, argv);
                return;
            
            default:
                SERVICE_PID = pid;
                break;
        }
        
        sleep (1); /* Prevent a respawn bomb */
        while (wait (&retval) != SERVICE_PID) {
            if (WATCHDOG_EXIT) break;
        }
    } while (! WATCHDOG_EXIT);
}

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
    pid_t pwrapper;
    uid_t svcuid;
    gid_t svcgid;
    struct passwd *pwd = NULL;
    FILE *pidfile;
    char pidbuf[128];
    
    if ((argc>1) && (strcmp (argv[1], "--foreground") == 0)) {
        (void) call (argc, argv);
        return 1;
    }
    
    /* Make sure we're not already running */
    pidfile = fopen (pidfilepath, "r");
    if (pidfile != NULL) {
        fread (pidbuf, 128, 1, pidfile);
        fclose (pidfile);
        pidbuf[127] = 0;
        pwatchdog = atoi (pidbuf);
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
    
    /* Create an outer fork that allows us to detach from the group */
    switch (pwrapper = fork()) {
        case -1:
            fprintf (stderr, "%%ERR%% Fork failed\n");
            break;
        
        case 0:
            /* Sever ties to active terminals */
            close (0);
            close (1);
            close (2);
            
            /* Fork off the watchdog process */
            switch (pwatchdog = fork()) {
                case -1:
                    exit (1);
        
                case 0:
                    pidfile = fopen (pidfilepath, "w");
                    fprintf (pidfile, "%i", pwatchdog);
                    fclose (pidfile);
                    watchdog_main (argc, argv, call);
                    exit (0);
                    break;
        
                default:
                    break;
            }
            break;
        
        default:
            exit (0);
    }
    return 0;
}
