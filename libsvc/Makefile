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

OBJS_LIBSVC = daemon.o
				  
all: ../lib/libsvc.a

../lib/libsvc.a: $(OBJS_LIBSVC)
	@echo "* LINK libsvc.a"
	@ar cr ../lib/libsvc.a $(OBJS_LIBSVC)

clean:
	rm -f *.o ../lib/libsvc.a

.SUFFIXES: .c .o
.c.o:
	@echo "* COMPILE $<"
	@$(CC) $(CFLAGS) -I../include -c $<
