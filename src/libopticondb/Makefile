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

OBJS_LIBOPTICONDB = db.o db_local.o
				  
all: ../../lib/libopticondb.a

../../lib/libopticondb.a: $(OBJS_LIBOPTICONDB)
	@echo "[LINK ] libopticondb.a"
	@ar cr ../../lib/libopticondb.a $(OBJS_LIBOPTICONDB)

clean:
	rm -f *.o ../../lib/libopticondb.a

.SUFFIXES: .c .o
.c.o:
	@echo "[BUILD] $<"
	@$(CC) $(CFLAGS) -I../../include -c $<
