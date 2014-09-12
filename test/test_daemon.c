#include <libsvc/daemon.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

int daemonfunc (int argc, const char *argv[]) {
    char buf[128];
    int i;
    FILE *f = fopen ("./.tmp-file","r");
    if (! f) {
        f = fopen ("./.tmp-file","w");
        fprintf (f, "%i", 1);
        fclose (f);
        return 0;
    }
    fgets (buf, 128, f);
    fclose (f);
    i = atoi (buf);
    if (i == 1) {
        f = fopen ("./.tmp-file","w");
        fprintf (f, "2");
        fclose (f);
    }
    while (1) sleep (60);
}

int main (int argc, const char *argv[]) {
    char buf[128];
    pid_t p;
    FILE *f;
    
    if (! daemonize ("./.tmp.pid", argc, argv, daemonfunc)) return 1;
    sleep (2);
    
    f = fopen ("./.tmp.pid", "r");
    if (! f) {
        fprintf (stderr, "Could not open pidfile\n");
        return 1;
    }
    fgets (buf, 128, f);
    fclose (f);
    
    p = atoi (buf);
    printf ("Killing %i\n", p);
    kill (p, SIGTERM);
    sleep (3);
    f = fopen ("./.tmp.pid","r");
    if (f) {
        unlink ("./.tmp-file");
        fprintf (stderr, "Pidfile not cleaned\n");
        return 1;
    }
    f = fopen ("./.tmp-file","r");
    if (! f) {
        fprintf (stderr, "Tempfile not found\n");
        return 1;
    }
    fgets (buf, 128, f);
    fclose (f);
    unlink ("./.tmp-file");
    if (atoi (buf) != 2) {
        fprintf (stderr, "Tempfile not rewritten: %i\n", atoi(buf));
    }
    
    return 0;
}
