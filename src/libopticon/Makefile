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

OBJS_TEST = test/test_roundtrip_encoding.o
OBJS_LIBOPTICON = aes.o host.o tenant.o \
				  util.o auth.o base64.o import.o \
				  ioport.o ioport_file.o \
				  ioport_buffer.o codec.o \
				  codec_json.o codec_pkt.o \
				  compress.o pktwrap.o \
				  watchlist.o defaults.o \
				  dump.o var.o react.o parse.o \
				  cliopt.o daemon.o log.o thread.o transport.o \
				  transport_udp.o packetqueue.o
all: ../../lib/libopticon.a

../../lib/libopticon.a: $(OBJS_LIBOPTICON)
	@echo "[LINK ] libopticon.a"
	@ar cr ../../lib/libopticon.a $(OBJS_LIBOPTICON)

clean:
	rm -f *.o ../../lib/libopticon.a

.SUFFIXES: .c .o
.c.o:
	@echo "[BUILD] $<"
	@$(CC) $(CFLAGS) -I../../include -c $<
