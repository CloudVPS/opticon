#ifndef _OPTIONS_H
#define _OPTIONS_H 1

typedef struct apioptions_s {
    const char  *dbpath;
    int          loglevel;
    const char  *logpath;
    const char  *confpath;
    const char  *mconfpath;
    const char  *pidfile;
    const char  *keystone_url;
    int          foreground;
    int          port;
    uuid         admintoken;
    var         *conf;
    var         *mconf;
} apioptions;

extern apioptions OPTIONS;

#endif
