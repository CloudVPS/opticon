#include <libopticon/thread.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

int COUNTER=0;
int CLEANED=0;

void cleanup (thread *t) {
    CLEANED++;
}

void run (thread *t) {
    COUNTER++;
    sleep (4);
    COUNTER++;
}

int main (int argc, const char *argv[]) {
    thread *t = thread_create (run, cleanup);
    while (! COUNTER) sleep(1);
    thread_cancel (t);
    assert (COUNTER == 1);
    assert (CLEANED == 1);
    return 0;
}
