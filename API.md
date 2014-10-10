Opticon API Reference
=====================

| **Path**                         | **Method**   | **Description**                                                            |
| -------------------------------- | ------------ | -------------------------------------------------------------------------- |
| /                                | GET          | Sends back a list of tenants available to the user                         |
| /token                           | GET          | Can be used by clients to verify they still have valid access              |
| /session                         | GET          | (ADMIN) Sends an overview of active collector sessions                     |
| /{TENANT}                        | GET          | Get information about a tenant (name, AES key)                             |
| /{TENANT}                        | POST         | Create a new tenant                                                        |
| /{TENANT}                        | PUT          | Update a tenant                                                            |
| /{TENANT}                        | DELETE       | Delete a tenant                                                            |
| /{TENANT}/meta                   | GET          | Sends back tenant's metadata                                               |
| /{TENANT}/meta                   | PUT/POST^1   | ​Update tenant's metadata                                                   |
| /{TENANT}/summary                | GET          | Get tenant meter summaries                                                 |
| /{TENANT}/meter                  | GET          | Sends a list of all default and custom meters active for the tenant        |
| /{TENANT}/meter/{MID}            | PUT/POST^1   | Create or update a custom meter definition                                 |
| /{TENANT}/meter/{MID}            | DELETE       | Remove a meter definition (and associated watchers)                        |
| /{TENANT}/watcher                | GET          | Sends a list of all default and custom watchers                            |
| /{TENANT}/watcher/{MID}          | PUT/POST^1   | Create or update a watcher                                                 |
| /{TENANT}/watcher/{MID}          | DELETE       | Remove a watcher (host-level watchers may still apply)                     |
| /{TENANT}/host                   | GET          | Sends a list of all the tenant's hosts                                     |
| /{TENANT}/host/overview          | GET          | Get an overview of host data                                               |
| /{TENANT}/host/{H}               | GET          | Sends the current record for the host                                      |
| /{TENANT}/host/{H}/watcher       | GET          | Sends a list of all default and custom watchers (host & tenant)            |
| /{TENANT}/host/{H}/watcher/{MID} | PUT/POST^1   | Create or update a host-level watcher                                      |
| /{TENANT}/host/{H}/watcher/{MID} | DELETE       | Remove a host-level watcher                                                |

^1: Both POST and PUT have the same effect. Sorry, REST purists.

Request Headers
---------------

The following headers have a special meaning:

| Header                  | Function                           |
| ----------------------- | ---------------------------------- |
| X-Opticon-Token         | Token for access the admin API     |
| X-Auth-Token            | Keystone token                     |

Error Replies
-------------

**INPUT:** Something stupid

**OUTPUT:** HTTP error, plus JSON data with a specific error message:

```javascript
{
    "error": "You're stupid"
}
```

**STATUS** **CODES:**

| Code | Description                                     |
| ---- | ----------------------------------------------- |
| 400  | The API server didn't like the data you sent    |
| 401  | Your didn't send a token, or it was invalid     |
| 403  | The server wanted an admin. You're not an admin |
| 404  | Requested object could not be found             |
| 405  | You sent an HTTP method that made no sense      |
| 409  | Resource you're trying to create already exists |
| 500  | Opticon dun goofed                              |

GET /
-----

**INPUT:** None

**OUTPUT:**

```javascript
{
    "tenant": [
        {
            "id": "001b7153-4f4b-4f1c-b281-cc06b134f98f",
            "name": "compute-pim",
            "hostcount": 3
        }
    ]
}
```

GET /token
----------

**INPUT:** None

**OUTPUT:**

```javascript
{
    "token": {
        "userlevel": "AUTH_USER"
    }
}
```

GET /session
------------

**INPUT:** None

**OUTPUT:**

```javascript
{
    "session": [
        {
            "tenantid": "001b7153-4f4b-4f1c-b281-cc06b134f98f",
            "hostid": "0d19d114-55c8-4077-9cab-348579c70612",
            "addr": 1008430172,
            "sessid": 1325435585,
            "lastcycle": "2014-10-05T11:44:41Z",
            "lastserial": 1412498933,
            "remote": "::ffff:192.168.1.1"
        },
        {
            "tenantid": "001b7153-4f4b-4f1c-b281-cc06b134f98f",
            "hostid": "65d13b37-41d0-4579-9331-ed4ed4c01259",
            "addr": 3316034127,
            "sessid": 2716013145,
            "lastcycle": "2014-10-05T11:45:43Z",
            "lastserial": 1412445083,
            "remote": "::ffff:192.168.1.2"
        }
    ]
}
```

GET /{TENANT}
-------------

**INPUT:** None

**OUTPUT:**

```javascript
{
    "tenant": {
        "key": "fAM9aZdUoNzdytgqLoS2y44dfZeqWeBY9wkGWAq72C4=",
        "name": "compute-pim",
        "meta": {
            "alles": "tof"
        }
    }
}
```

POST /{TENANT}
--------------
**INPUT:** 

```javascript
{
    "tenant": {
        "key": "fAM9aZdUoNzdytgqLoS2y44dfZeqWeBY9wkGWAq72C4=",
        "name": "cowboy-henk"
    }
}
```

**ALTINPUT:**

```javascript
{
    "tenant": {}
}
```

**OUTPUT:**

```javascript
{
    "tenant": {
        "key": "fAM9aZdUoNzdytgqLoS2y44dfZeqWeBY9wkGWAq72C4=",
        "name": "cowboy-henk"
    }
}
```

**NOTES:**

1. Users authenticated through keystone are not allowed to set their own `name` 
   field. This field will always be inherited from the keystone metadata.
2. If no `key` field is provided, the system will generate one. This is the 
   recommended way to get one on systems that have no secure random.

PUT /{TENANT}
-------------
**INPUT:** 

```javascript
{
    "tenant": {
        "key": "fAM9aZdUoNzdytgqLoS2y44dfZeqWeBY9wkGWAq72C4=",
        "name": "cowboy-henk"
    }
}
```

**OUTPUT:** Irrelevant

**NOTES:**

1. Users authenticated through keystone are not allowed to set their own `name` 
   field. This field will always be inherited from the keystone metadata.

DELETE /{TENANT}
----------------
**INPUT:** None

**OUTPUT:** Irrelevant

GET /{TENANT}/meta
------------------

**INPUT:** None

**OUTPUT:**

```javascript
{
    "metadata": {
        "alles": "tof"
    }
}
```

POST /{TENANT}/meta
--------------------

**INPUT:**

```javascript
{
    "metadata": {
        "key": "value"
    }
}
```

**OUTPUT:** Irrelevant

GET /{TENANT}/summary
---------------------

**INPUT:** None

**OUTPUT:**

```javascript
{
    "summary": {
        "cpu": 13.276786,
        "warning": 2,
        "alert": 0,
        "critical": 0,
        "stake": 0,
        "netin": 548,
        "netout": 790
    }
}
```

GET /{TENANT}/meter
-------------------

**INPUT:** None

**OUTPUT:**

```javascript
{
    "meter": {
        "agent/ip": {
            "type": "string",
            "description": "Remote IP Address"
        },
        "os/kernel": {
            "type": "string",
            "description": "Version"
        },
        "os/arch": {
            "type": "string",
            "description": "CPU Architecture"
        },

...

        "who/user": {
            "type": "string",
            "description": "User",
            "origin": "tenant"
        },
        "who/tty": {
            "type": "string",
            "description": "TTY",
            "origin": "tenant"
        },
        "who/remote": {
            "type": "string",
            "description": "Remote IP",
            "origin": "tenant"
        },
        "who": {
            "type": "table",
            "description": "Remote Users",
            "origin": "tenant"
        }
    }
}
```

**NOTES:**

1. This is a combination view. Meters that are defined at the tenant
   leve will have an `origin` field set to `"tenant"`. 
   
POST /{TENANT}/meter/{METER}
----------------------------

**INPUT:**

```javascript
{
    "meter": {
        "type": "{METERTYPE}",
        "description": "My Meter",
        "unit": "fl/mftn"
    }
}
```

**OUTPUT:** Irrelevant

**NOTES:**

1. The `unit` field is optional.
2. Technically, so is `description`, but don't be a dick.

DELETE /{TENANT}/meter/{METER}
------------------------------

**INPUT:** None

**OUTPUT:** Irrelevant

GET /{TENANT}/watcher
---------------------

**INPUT:** None

**OUTPUT:**

```javascript
{
    "watcher": {
        "df/pused": {
            "type": "frac",
            "warning": {
                "cmp": "gt",
                "val": 90.000000,
                "weight": 1.000000
            },
            "alert": {
                "cmp": "gt",
                "val": 95.000000,
                "weight": 1.000000
            },
            "critical": {
                "cmp": "gt",
                "val": 99.000000,
                "weight": 1.000000
            }
        },
        "pcpu": {
            "type": "frac",
            "warning": {
                "cmp": "gt",
                "val": 45.000000,
                "weight": 1.000000,
                "origin": "tenant"
            },
            "alert": {
                "cmp": "gt",
                "val": 50.000000,
                "weight": 1.000000
            },
            "critical": {
                "cmp": "gt",
                "val": 99.000000,
                "weight": 1.000000
            }
        },
...
    }
}
```

**NOTES:**

1. This is a combination view. Watchers that are defined at the tenant
   level will have their `origin` field set to `tenant`.

POST /{TENANT}/watcher/{METER}
------------------------------

**INPUT:**
```javascript
{
    "watcher": {
        "warning":{
            "cmp": {MATCHFUNCTION},
            "val": {MATCHVALUE},
            "weight": 1.0
        },
        "alert":{
            "cmp": {MATCHFUNCTION},
            "val": {MATCHVALUE},
            "weight": 1.0
        },
        "critical":{
            "cmp": {MATCHFUNCTION},
            "val": {MATCHVALUE},
            "weight": 1.0
        }
    }
}
```

**NOTES:**

1. The alert levels `warning`, `alert`, and `critical` are all optional.
   If only a `warning` is defined, the watcher settings for the other
   two levels are unaffected by the update.
2. Valid matchfunctions for `int` and `frac` types are `lt`, and `gt.
   For string types, `eq` is available.
3. If no `weight` is provided, a default weight of `1.0` is chosen.

DELETE /{TENANT}/watcher/{METER}
--------------------------------

**INPUT:** None

**OUTPUT:** Irrelevant

GET /{TENANT}/host
------------------

**INPUT:** None

**OUTPUT:**

```json
{
    "host": [
        {
            "id": "65d13b37-41d0-4579-9331-ed4ed4c01259",
            "usage": 1156821,
            "start": "2014-10-03T17:18:00",
            "end": "2014-10-05T13:13:00"
        },
        {
            "id": "0d19d114-55c8-4077-9cab-348579c70612",
            "usage": 1704224,
            "start": "2014-10-03T17:03:00",
            "end": "2014-10-05T13:13:00"
        },
        {
            "id": "2b331038-aac4-4d8b-a7cd-5271b603bd1e",
            "usage": 1663072,
            "start": "2014-10-03T17:15:00",
            "end": "2014-10-05T13:13:00"
        }
    ]
}
```

**NOTES:**

1. Times are in UTC
2. The `usage` field describes the database size, in bytes.

GET /{TENANT}/host/overview
---------------------------

**INPUT:** None

**OUTPUT:**

```javascript
{
    "overview": {
        "65d13b37-41d0-4579-9331-ed4ed4c01259": {
            "status": "WARN",
            "pcpu": 7.406250,
            "loadavg": 1.406250,
            "net/in_kbs": 1,
            "net/in_pps": 1,
            ...
        }
        "1caf0cbc-0968-478d-b17d-d3dd63dc8d11": {
            "status": "OK",
            "pcpu": 2.15,
            "loadavg": 0.02188,
            ...
        }
        ...
    }
}
```

GET /{TENANT}/host/{HOST}
-------------------------

**INPUT:** None

**OUTPUT:**

```javascript
{
    "status": "WARN",
    "problems": [
        "proc/stuck",
        "df/pused"
    ],
    "badness": 45.438000,
    "agent": {
        "ip": "::ffff:92.108.228.195"
    },
    "hostname": "giskard.local",
    "os": {
        "kernel": "Darwin",
        "version": "14.0.0",
        "arch": "x86_64"
    },
...
}
```

**NOTES:**

1. The `status`, `problems`, `badness`, and `agent` records originated
   from `opticon-collector`, not the origin host.

GET /{TENANT}/host/{HOST}/watcher
---------------------------------

**INPUT:** None

**OUTPUT:** Same as /{TENANT}/watcher

**NOTES:** 

1. An extra `origin` option is now `host` for watchers overridden at the
   host-level.
   
POST /{TENANT}/host/{HOST}/watcher/{METER}
------------------------------------------

**INPUT:** 

```javascript```
{
    "watcher": {
        "warning": {
            "value": {MATCHVALUE},
            "weight": 1.0
        }
    }
}
```

**NOTES:**

1. This call differs from the one at the tenant level, in that it can only
   act as an adjustment of an existing watcher higher up the tree. This
   is why there is no `cmp` field. If the watcher is not in the default
   list of watchers, it needs to be defined with the proper `cmp` function
   at the tenant level, before any host-level watchers make sense.
2. Like in /{TENANT}/watcher, only the trigger levels that are posted
   are updated.

DELETE /{TENANT}/host/{HOST}/watcher/{METER}
--------------------------------------------

**INPUT:** None

**OUTPUT:** Irrelevant
