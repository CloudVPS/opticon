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

OBJS_TEST = host.o tenant.o test.o util.o auth.o base64.o ioport.o codec.o

all: test

test: $(OBJS_TEST)
	$(CC) -o test $(OBJS_TEST)

clean:
	rm -f *.o test

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -I. -c $<
