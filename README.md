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

**opticon-db**

Query and maintenance tool for the opticon database backend.

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
configuration is the tenant database.

### Creating a new tenant

The *opticon-db* tool can be used to add tenants to the database. Use the
following command to create a new tenant:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ opticon-db tenant-create --name "Acme"
Tenant created:
------------------------------------------------------------------
     Name: Acme
     UUID: dbe7c559-297e-e65b-9eca-fc96037c67e2
  AES Key: nwKT5sfGa+OlYHwa7rZZ7WQaMsAIEWKQii0iuSUPfG0=
------------------------------------------------------------------
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The tool will spit out the UUID for the newly created tenant, as well as the
tenant AES256 key to be used in the configuration of this tenant’s
*opticon-agent* instances.

If you want to create a tenant with a predefined UUID, you can use the —tenant
command line flag:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ opticon-db tenant-create --name "Acme" --tenant 0296d893-8187-4f44-a31b-bf3b4c19fc10 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Manipulating tenant metadata

Every tenant object in the opticon database has freeform metadata. Some of it is
used internally, like the tenant AES key. Use the *tenant-get-metadata
*sub-command to view a tenant’s metadata in JSON format:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ opticon-db tenant-get-metadata --tenant 0296d893-8187-4f44-a31b-bf3b4c19fc10
"key": "nwKT5sfGa+OlYHwa7rZZ7WQaMsAIEWKQii0iuSUPfG0=",
"name": "Acme"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can add keys to the metadata, or change the value of existing keys, by using
the obviously name *tenant-set-metadata* sub-command:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ opticon-db tenant-set-metadata --tenant 0296d893-8187-4f44-a31b-bf3b4c19fc10 storpel "extra crispy"
$ opticon-db tenant-get-metadata --tenant 0296d893-8187-4f44-a31b-bf3b4c19fc10
"key": "nwKT5sfGa+OlYHwa7rZZ7WQaMsAIEWKQii0iuSUPfG0=",
"name": "Acme",
"storpel": "extra crispy"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
