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

ifeq (darwin,$(OSNAME))
  LDFLAGS+=-L/usr/local/lib -framework Security -framework CoreFoundation
endif

CFLAGS+=-DOSNAME=$(OSNAME) -DOSREL=$(OSREL) -DOSRELMAJOR=$(OSRELMAJOR)

ifeq (linux,$(OSNAME))
  CFLAGS+=-std=c99 -D_GNU_SOURCE
  LIBS+=-lm
endif

OBJS_OPTICONCLI = cmd.o main.o api.o prettyprint.o
				  
all: ../../bin/opticon

../../bin/opticon: $(OBJS_OPTICONCLI) $(LIBS)
	@echo "[LINK ] opticon-cli"
	@$(CC) $(LDFLAGS) -o ../../bin/opticon $(OBJS_OPTICONCLI) $(LIBS) $(LIBCURL) -lm -lz

clean:
	rm -f *.o ../../bin/opticon

.SUFFIXES: .c .o
.c.o:
	@echo "[BUILD] $<"
	@$(CC) $(CFLAGS) $(INCLUDES_CURL) -I../../include -c $<
