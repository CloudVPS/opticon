#ifndef _NOTIFY_H
#define _NOTIFY_H

typedef struct notification_s {
    struct notification_s   *next;
    struct notification_s   *prev;
    char                     problems[128];
    uuid                     hostid;
    bool                     notified;
    char                     status[16];
    time_t                   lastchange;
} notification;

typedef struct notifylist_s {
    notification        *first;
    notification        *last;
} notifylist;

#endif
