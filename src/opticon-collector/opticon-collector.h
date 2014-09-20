#ifndef _OPTICON_COLLECTOR_H
#define _OPTICON_COLLECTOR_H 1

#include <libopticon/datatypes.h>
#include <libopticon/codec.h>
#include <libopticondb/db.h>
#include <libopticonf/var.h>
#include <libsvc/packetqueue.h>
#include <libsvc/transport.h>

typedef struct appcontext_s {
    codec       *codec;
    db          *db;
    packetqueue *queue;
    intransport *transport;
    var         *conf;
    const char  *confpath;
    const char  *pidfile;
    int          foreground;
    int          listenport;
    const char  *listenaddr;
} appcontext;

extern appcontext APP;

#endif
