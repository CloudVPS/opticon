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

OBJS_OPTICONDB = cmd.o import.o main.o
LIBS = ../../lib/libopticon.a ../../lib/libopticondb.a
				  
all: ../../bin/opticon-db

../../bin/opticon-db: $(OBJS_OPTICONDB) $(LIBS)
	@echo "[LINK ] opticon-db"
	@$(CC) $(LDFLAGS) -o ../../bin/opticon-db $(OBJS_OPTICONDB) $(LIBS) -lz

clean:
	rm -f *.o ../../bin/opticon-db

.SUFFIXES: .c .o
.c.o:
	@echo "[BUILD] $<"
	@$(CC) $(CFLAGS) -I../../include -c $<
