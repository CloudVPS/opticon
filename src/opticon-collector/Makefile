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

OBJS_OPTICONCOLLECTOR = main.o
LIBS = ../../lib/libopticon.a ../../lib/libopticondb.a \
		../../lib/libsvc.a ../../lib/libopticonf.a
				  
all: ../../bin/opticon-collector

../../bin/opticon-collector: $(OBJS_OPTICONCOLLECTOR) $(LIBS)
	@echo "[LINK ] opticon-collector"
	@$(CC) $(LDFLAGS) -o ../../bin/opticon-collector \
		$(OBJS_OPTICONCOLLECTOR) $(LIBS) -lz

clean:
	rm -f *.o ../../bin/opticon-collector

.SUFFIXES: .c .o
.c.o:
	@echo "[BUILD] $<"
	@$(CC) $(CFLAGS) -I../../include -c $<
