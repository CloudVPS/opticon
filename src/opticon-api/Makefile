include ../../makeinclude

ifdef STATIC
  CFLAGS+=-DSTATIC
  LDFLAGS+=-static
endif

OSNAME?=$(shell uname -s | tr A-Z a-z)
OSREL?=$(shell uname -r)
OSRELMAJOR?=$(shell uname -r | cut -f1 -d.)

ifeq (freebsd,$(OSNAME))
  LDFLAGS+=-lkvm
endif

LIBS = ../../lib/libopticondb.a ../../lib/libhttp.a ../../lib/libopticon.a 

ifeq (linux,$(OSNAME))
  CFLAGS+=-std=c99 -D_GNU_SOURCE
  LDFLAGS+=-lpthread -lm
endif

CFLAGS+=-DOSNAME=$(OSNAME) -DOSREL=$(OSREL) -DOSRELMAJOR=$(OSRELMAJOR)

OBJS_OPTICONAPI = cmd_err.o cmd_host.o cmd_tenant.o \
				  main.o req_context.o tokencache.o
				  
all: ../../bin/opticon-api

../../bin/opticon-api: $(OBJS_OPTICONAPI) $(LIBS)
	@echo "[LINK ] opticon-api"
	@$(CC) $(LDFLAGS) -o ../../bin/opticon-api $(OBJS_OPTICONAPI) $(LIBS) \
		   $(LIBMICROHTTPD) $(LIBCURL) $(LPTHREAD) -lm -lz 

clean:
	rm -f *.o ../../bin/opticon-api

.SUFFIXES: .c .o
.c.o:
	@echo "[BUILD] $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -I../../include -c $<
