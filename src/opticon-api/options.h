#ifndef _OPTIONS_H
#define _OPTIONS_H 1

typedef struct apioptions_s {
    const char *dbpath;
    int port;
    uuid admintoken;
} apioptions;

extern apioptions OPTIONS;

#endif
