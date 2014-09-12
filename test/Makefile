ifdef STATIC
  CFLAGS+=-DSTATIC
  LDFLAGS+=-static
endif

LUA?=lua
LUALIBS?=$(shell pkg-config --libs $(LUA))
LUAINC?=$(shell pkg-config --cflags $(LUA))

OSNAME?=$(shell uname -s | tr A-Z a-z)
OSREL?=$(shell uname -r)
OSRELMAJOR?=$(shell uname -r | cut -f1 -d.)

ifeq (freebsd,$(OSNAME))
  LDFLAGS+=-lkvm
endif

CFLAGS+=-DOSNAME=$(OSNAME) -DOSREL=$(OSREL) -DOSRELMAJOR=$(OSRELMAJOR)

all: test_aes test_daemon test_localdb test_roundtrip_encoding test_ioport test_udp

test_aes: test_aes.o
	@echo "* LINK test_aes"
	@$(CC) -o test_aes test_aes.o \
		  ../lib/libopticon.a -lz

test_daemon: test_daemon.o
	@echo "* LINK test_daemon"
	@$(CC) -o test_daemon test_daemon.o \
		  ../lib/libsvc.a ../lib/libopticon.a -lz

test_localdb: test_localdb.o
	@echo "* LINK test_localdb"
	@$(CC) -o test_localdb test_localdb.o \
		  ../lib/libopticon.a ../lib/libopticondb.a -lz

test_ioport: test_ioport.o
	@echo "* LINK test_ioport"
	@$(CC) -o test_ioport test_ioport.o \
		  ../lib/libopticon.a -lz

test_roundtrip_encoding: test_roundtrip_encoding.o
	@echo "* LINK test_roundtrip_encoding"
	@$(CC) -o test_roundtrip_encoding test_roundtrip_encoding.o \
		  ../lib/libopticon.a -lz

test_udp: test_udp.o
	@echo "* LINK test_udp"
	@$(CC) -o test_udp test_udp.o \
		  ../lib/libopticon.a ../lib/libsvc.a -lz

clean:
	rm -f *.o test_roundtrip_encoding

.SUFFIXES: .c .o
.c.o:
	@echo "* COMPILE $<"
	@$(CC) $(CFLAGS) -I../include -c $<
