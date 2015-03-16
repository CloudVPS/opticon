#ifndef _NOTIFY_H
#define _NOTIFY_H

#include <stdbool.h>
#include <stdint.h>
#include <libopticon/uuid.h>
#include <time.h>

/* =============================== TYPES =============================== */

typedef struct notification_s {
    struct notification_s   *next;
    struct notification_s   *prev;
    uuid                     hostid;
    bool                     notified;
    bool                     isproblem;
    char                     status[16];
    time_t                   lastchange;
} notification;

typedef struct notifylist_s {
    notification        *first;
    notification        *last;
} notifylist;

/* ============================= FUNCTIONS ============================= */

notification *notification_create (void);
void          notification_delete (notification *);
void          notifylist_init (notifylist *);
void          notifylist_clear (notifylist *);
void          notifylist_remove (notifylist *, notification *);
void          notifylist_link (notifylist *, notification *);
notification *notifylist_find (notifylist *, uuid);
bool          notifylist_check_actionable (notifylist *);
notification *notifylist_find_overdue (notifylist *self, notification *n);

#endif
