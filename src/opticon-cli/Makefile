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

LIBS = ../../lib/libopticon.a ../../lib/libopticondb.a ../../lib/libhttp.a
LIBCURL = -lcurl

ifeq (darwin,$(OSNAME))
  CFLAGS+=-I/usr/local/include
  LDFLAGS+=-L/usr/local/lib -framework Security -framework CoreFoundation
  LIBCURL=/usr/local/lib/libcurl.a
endif

CFLAGS+=-DOSNAME=$(OSNAME) -DOSREL=$(OSREL) -DOSRELMAJOR=$(OSRELMAJOR)

OBJS_OPTICONCLI = cmd.o main.o api.o
				  
all: ../../bin/opticon

../../bin/opticon: $(OBJS_OPTICONCLI) $(LIBS)
	@echo "[LINK ] opticon-cli"
	$(CC) $(LDFLAGS) -o ../../bin/opticon $(OBJS_OPTICONCLI) $(LIBS) $(LIBCURL) -lz

clean:
	rm -f *.o ../../bin/opticon

.SUFFIXES: .c .o
.c.o:
	@echo "[BUILD] $<"
	@$(CC) $(CFLAGS) -I../../include -c $<
