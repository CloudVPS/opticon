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

all: test_roundtrip_encoding

test_roundtrip_encoding: test_roundtrip_encoding.o
	$(CC) -o test_roundtrip_encoding test_roundtrip_encoding.o \
		  ../lib/libopticon.a -lz

clean:
	rm -f *.o test_roundtrip_encoding

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -I../include -c $<
