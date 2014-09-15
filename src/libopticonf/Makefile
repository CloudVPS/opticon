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

OBJS_LIBOPTICONF = var.o react.o parse.o
				  
all: ../../lib/libopticonf.a

../../lib/libopticonf.a: $(OBJS_LIBOPTICONF)
	@echo "[LINK ] libopticondb.a"
	@ar cr ../../lib/libopticonf.a $(OBJS_LIBOPTICONF)

clean:
	rm -f *.o ../../lib/libopticonf.a

.SUFFIXES: .c .o
.c.o:
	@echo "[BUILD] $<"
	@$(CC) $(CFLAGS) -I../../include -c $<
