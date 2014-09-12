#include <libsvc/thread.h>
#include <stdio.h>
#include <unistd.h>

int COUNTER=0;

void run (thread *t) {
    COUNTER++;
}

int main (int argc, const char *argv[]) {
    thread *t = thread_create (run);
    while (! COUNTER) sleep(1);
    return 0;
}
