Opticon Server Metering for OpenStack
=====================================

Opticon is the CloudVPS successor to its N2 monitoring service: A way of keeping
tabs on what your servers are doing for purposes of monitoring, performance
analysis and incident forensics. Opticon is designed to be used in conjunction
with the OpenStack Compute service, but can be operated independently.

Like its predecessor, the Opticon server agent uses *push notification* to send
updates to the central collector, allowing important performance data to keep
flowing even under adversial conditions, where traditional pollers tend to fail.

Components
----------

Opticon consists of three main program components. Monitored servers run
*opticon-agent*. On the other side, *opticon-collector* and *opticon-api* handle
the collection of data and interaction with users respectively.

The source code is organized into smaller modules, some of which are shared
between multiple programs:

**libopticon**

Data structures for keeping track of hosts and meters, as well as encoding and
decoding them for transport or storage. Infrastructure for applications and
daemons.

**libopticondb**

Access to the timestamp-indexed opticon disk database that tracks metering
samples.

**opticon-api**

A HTTP service that offers a local and remote API for manipulating and querying
the opticon database.

**opticon-cli**

A command line client for opticon-api.

**opticon-agent**

Monitoring tool that runs on the client machines and sends the probe data.

**opticon-collector**

Receiving daemon that collects metering data and writes it to a database, while
also keeping track of alert-conditions.

Configuring opticon-collector
-----------------------------

There are two levels of configuration at play for the collector daemon. The
first level is its configuration file, which sets up some of the basics, and
defines the default set of meters, and alert levels. The second level of
configuration is the tenant database. The latter has to be configured through
the opticon cli, so we’ll get to that after setting up the API server.

### The configuration file

The collector has a very simple base configuration in
/etc/opticon/opticon-collector.conf, only dealing with system resources:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
network {
    port: 1047
    address: *
}
database {
    path: "/var/db/opticon"
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Additionally, system-wide custom meters and watchers can be configured in
/etc/opticon/opticon-meter.conf, like this:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Custom meter and watcher definitions
"pcpu" {
    type: frac
    description: "CPU Usage"
    unit: "%"
    warning { cmp: gt, val: 30.0, weight: 1.0 }
    alert { cmp: gt, val: 50.0, weight: 1.0 }
}
"hostname" {
    type: string
    description: "Hostname"
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The configuration files are in a ’sloppy’ JSON format. You can use strict JSON
formatting, but you can also leave out any colons, commas, or quotes around
strings that have no whitespace or odd characters.

Configuring opticon-api
-----------------------

The API server keeps its configuration in /etc/opticon/opticon-api.conf. This is
how it typically looks:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
network {
    port: 8888
}
auth {
    admin_token: a666ed1e-24dc-4533-acab-1efb2bb55081
    admin_host: 127.0.0.1
    keystone_url: "https://identity.stack.cloudvps.com/v2.0"
}
database {
    path: "/var/db/opticon"
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Generate a random UUID for the `admin_token` setting. Requests with this token
coming from the IP address specified by `admin_host` will be granted
administrative privileges.

The `keystone_url` should point to an OpenStack Keystone server. Authorization
tokens sent to the Opticon API using the Openstack `X-Auth-Token` header will be
verified against this service, and any OpenStack tenants the token is said to
have access to will be open to the local session.

Note that, in order to keep latency at a minimum, opticon-api will cache valid
and invalid tokens, and their associated tenant lists, for up to an hour.

The API server will also reference `/etc/opticon/opticon-meter.conf`, so it can
inform users of the actual defaults active.

Configuring the opticon client
------------------------------

The client gets its configuration from both /etc/opticon/opticon-cli.conf and
\$HOME/.opticonrc (the latter having precedence, but both files are parsed and
merged). Here is what it looks like:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
endpoints {
  keystone: "https://identity.stack.cloudvps.com/v2.0"
  opticon: "http://127.0.0.1:8888/"
}
defaults {
  tenant: 001b7153-4f4b-4f1c-b281-cc06b134f98f
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

At some time in the future, the opticon endpoint will be available directly
through keystone, and the `opticon` endpoint definition will become optional,
but for now it has to be in there. The tenant set up in defaults is the tenant
that will be used for any commands that aren’t given with an explicit `--tenant`
flag

Managing the tenant database
----------------------------

Now that the collector and API-server are active, and the client knows how to
talk to them, the admin API can be used to add tenants to the database. Use the
following command to create a new tenant:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ opticon --opticon-token a666ed1e-24dc-4533-acab-1efb2bb55081 \
  tenant-create --name "Acme"
Tenant created:
------------------------------------------------------------------
     Name: Acme
     UUID: dbe7c559-297e-e65b-9eca-fc96037c67e2
  AES Key: nwKT5sfGa+OlYHwa7rZZ7WQaMsAIEWKQii0iuSUPfG0=
------------------------------------------------------------------
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The `--opticon-token` flag is used to bypass KeyStone authentication and use the
admin API. The tool will spit out the UUID for the newly created tenant, as well
as the tenant AES256 key to be used in the configuration of this tenant’s
*opticon-agent* instances.

If you want to create a tenant with a predefined UUID, you can use the --tenant
command line flag:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ opticon --opticon-token a666ed1e-24dc-4533-acab-1efb2bb55081 \
  tenant-create --name "Acme" --tenant 0296d893-8187-4f44-a31b-bf3b4c19fc10 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This will be followed by the same information as the first example.

### Getting an overview of tenants

If accessed through the admin API, the `tenant-list` sub-command will show
information about all tenants on the system:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ opticon --opticon-token a666ed1e-24dc-4533-acab-1efb2bb55081 tenant-list
UUID                                 Hosts  Name
--------------------------------------------------------------------------------
001b7153-4f4b-4f1c-b281-cc06b134f98f     2  compute-pim
0296d893-8187-4f44-a31b-bf3b4c19fc10     0  Acme
6c0606c4-97e6-37dc-14fc-b7c1c61effef     0  compute-demo
--------------------------------------------------------------------------------
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The same command issued to the regular API will restrict this list to tenants
accessible to the user.

### Deleting a tenant

To get rid of a tenant (and reclaim all associated storage), use the
`tenant-delete` sub-command:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ opticon --opticon-token a666ed1e-24dc-4533-acab-1efb2bb55081 \
  tenant-delete --tenant 0296d893-8187-4f44-a31b-bf3b4c19fc10
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

that should teach them.

Configuring opticon-agent
-------------------------

With a tenantid and access key in hand, you can now go around and install
opticon-agent on servers that you would like to monitor. The agent reads its
configuration from `/etc/opticon/opticon-agent.conf`. First let’s take a look at
a rather straightforward configuration:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
collector {
    config: manual
    address: 192.168.1.1
    port: 1047
    key: "nwKT5sfGa+OlYHwa7rZZ7WQaMsAIEWKQii0iuSUPfG0="
    tenant: "001b71534f4b4f1cb281cc06b134f98f"
    host: "0d19d114-55c8-4077-9cab-348579c70612"
}
probes {
    top {
        type: built-in
        call: probe_top
        interval: 60
    }
    hostname {
        type: built-in
        call: probe_hostname
        interval: 300
    }
    uname {
        type: built-in
        call: probe_uname
        interval: 300
    }
    df {
        type: built-in
        call: probe_df
        interval: 300
    }
    uptime {
        type: built-in
        call: probe_uptime
        interval: 60
    }
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The `collector` section tells the agent how to reach the opticon-collector, and
how to identify itself. If the server runs in an OpenStack cloud, you can change
the `config` setting from `manual` to `metadata`. This will tell the agent to
get the connection information out of the OpenStack metadata service, instead.
It will try to read the following metadata fields: `opticon_collector_address`,
`opticon_collector_port`, `opticon_tenant_key`, `opticon_tenant_id`, and
`opticon_host_id`.

If you are using manual configuration, be sure to use a unique, random, UUID for
the `host` field. You can use the `uuidgen` tool on most UNIX command lines to
get a fresh one.

The `probes` section defines the metering probes that the agent shall run and
the frequency of updates. The agent sends out metering data over two channels:
The fast lane, and the slow lane. Data that isn’t subject to rapid change should
take the slow lane path, that gets sent out every 300 seconds. The `interval`
setting determines the amount of seconds between individual samples for a probe.
Note that this probing happens independently of packet scheduling, so setting up
intervals other than `60` or `300` is of limited use.

Accessing opticon as a user
---------------------------

After you used the admin API to create a tenant, you should be able to access
the rest of the functionality from any machine running an opticon client. If you
invoke *opticon* without a `--opticon-token` flag, the first time it wants to
make contact with the API server, it will ask you for keystone credentials:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ opticon tenant-list
% Login required

  Openstack Domain: identity.stack.cloudvps.com
  Username........: pi
  Password........: 

UUID                                 Hosts  Name
--------------------------------------------------------------------------------
001b7153-4f4b-4f1c-b281-cc06b134f98f     2  compute-pim
--------------------------------------------------------------------------------
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The next time you issue a request, the client will use a cached version of the
Keystone token it acquired with your username and password. If you’re having
issues with your key, you can remove the cache file manually, it is stored in
`$HOME/.opticon-token-cache`.

Since this account only has one tenant, for the rest of this documentation, we
will assume that this tenant-id is specified in `.opticonrc`. In situations
where you have to deal with multiple tenants, the `--tenant` flag can added to
any command to indicate a specific tenant.

### Manipulating tenant metadata

Every tenant object in the opticon database has freeform metadata. Some of it is
used internally, like the tenant AES key. Use the `tenant-get-metadata`
sub-command to view a tenant’s metadata in JSON format:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ opticon tenant-get-metadata
{
    "metadata": {
    }
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can add keys to the metadata, or change the value of existing keys, by using
the obviously named `tenant-set-metadata` sub-command:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ opticon tenant-set-metadata sleep optional
$ opticon tenant-get-metadata
{
    "metadata": {
        "sleep": "optional"
    }
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
