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
| /{TENANT}/meta                   | PUT/POST^1   | ​Update tenant's metadata                                                  |
| /{TENANT}/meter                  | GET          | Sends a list of all default and custom meters active for the tenant        |
| /{TENANT}/meter/{MID}            | PUT/POST^1   | Create or update a custom meter definition                                 |
| /{TENANT}/meter/{MID}            | DELETE       | Remove a meter definition (and associated watchers)                        |
| /{TENANT}/watcher                | GET          | Sends a list of all default and custom watchers                            |
| /{TENANT}/watcher/{MID}          | PUT/POST^1   | Create or update a watcher                                                 |
| /{TENANT}/watcher/{MID}          | DELETE       | Remove a watcher (host-level watchers may still apply)                     |
| /{TENANT}/host                   | GET          | Sends a list of all the tenant's hosts                                     |
| /{TENANT}/host/{H}               | GET          | Sends the current record for the host                                      |
| /{TENANT}/host/{H}/watcher       | GET          | Sends a list of all default and custom watchers (host & tenant)            |
| /{TENANT}/host/{H}/watcher/{MID} | PUT/POST^1   | Create or update a host-level watcher                                      |
| /{TENANT}/host/{H}/watcher/{MID} | DELETE       | Remove a host-level watcher                                                |

^1: Both POST and PUT have the same effect. Sorry, REST purists.

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

**INPUT:** Irrelevant

**OUTPUT:** Irrelevant
