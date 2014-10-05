Opticon API Reference
=====================

| **Path**                         | **Method**   | **Description**                                                            |
| -------------------------------- | ------------ | -------------------------------------------------------------------------- |
| /{TENANT}/meter                  | GET          | Sends a list of all default and custom meters active for the tenant        |
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

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
{
    "error": "You're stupid"
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

GET /
-----

**INPUT:** None

**OUTPUT:**

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
{
    "tenant": [
        {
            "id": "001b7153-4f4b-4f1c-b281-cc06b134f98f",
            "name": "compute-pim",
            "hostcount": 3
        }
    ]
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

GET /token
----------

**INPUT:** None

**OUTPUT:**

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
{
    "token": {
        "userlevel": "AUTH_USER"
    }
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

GET /session
------------

**INPUT:** None

**OUTPUT:**

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

GET /{TENANT}
-------------

**INPUT:** None

**OUTPUT:**

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
{
    "tenant": {
        "key": "fAM9aZdUoNzdytgqLoS2y44dfZeqWeBY9wkGWAq72C4=",
        "name": "compute-pim",
        "meta": {
            "alles": "tof"
        }
    }
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
