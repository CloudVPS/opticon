#include <libsvc/daemon.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>

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
    
    assert (daemonize ("./.tmp.pid", argc, argv, daemonfunc));
    sleep (2);
    
    assert (f = fopen ("./.tmp.pid", "r"));
    fgets (buf, 128, f);
    fclose (f);
    
    p = atoi (buf);
    assert (kill (p, SIGTERM) == 0);
    sleep (3);
    assert (fopen ("./.tmp.pid","r") == NULL);
    assert (f = fopen ("./.tmp-file","r"));
    fgets (buf, 128, f);
    fclose (f);
    unlink ("./.tmp-file");
    assert (atoi (buf) == 2);
    return 0;
}
