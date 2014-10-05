#include <libopticon/daemon.h>
#include <libopticon/log.h>
#include <libopticon/uuid.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>

char TMPF[128];

int daemonfunc (int argc, const char *argv[]) {
    char buf[128];
    int i;
    FILE *f = fopen (TMPF,"r");
    if (! f) {
        f = fopen (TMPF,"w");
        fprintf (f, "%i", 1);
        fclose (f);
        return 0;
    }
    fgets (buf, 128, f);
    fclose (f);
    i = atoi (buf);
    if (i == 1) {
        f = fopen (TMPF,"w");
        fprintf (f, "2");
        fclose (f);
    }
    while (1) sleep (60);
}

int main (int argc, const char *argv[]) {
    char buf[128];
    char PIDF[128];
    pid_t p;
    FILE *f;
    
    uuid u = uuidgen();
    uuid2str (u, buf);
    sprintf (PIDF, "/tmp/%s.pid", buf);
    sprintf (TMPF, "/tmp/%s.file", buf);
    
    assert (daemonize (PIDF, argc, argv, daemonfunc, 0));
    log_info ("Giving the daemon some time");
    sleep (2);
    
    assert (f = fopen (PIDF, "r"));
    fgets (buf, 128, f);
    fclose (f);
    
    log_info ("Terminating daemon");
    p = atoi (buf);
    assert (kill (p, SIGTERM) == 0);
    sleep (1);
    log_info ("Checking on leftovers");
    f = fopen (PIDF,"r");
    int cnt = 0;
    while (cnt < 5 && f) {
        fclose (f);
        sleep (1);
        f = fopen (PIDF,"r");
        cnt++;
    }
    assert (f == NULL);
    assert (f = fopen (TMPF,"r"));
    fgets (buf, 128, f);
    fclose (f);
    unlink (TMPF);
    assert (atoi (buf) == 2);
    return 0;
}
