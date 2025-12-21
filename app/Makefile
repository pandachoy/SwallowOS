DEFAULT_HOST!=../default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

CFLAGS?=-O0 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR=$(SYSROOT)
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
INCLUDEDIR?=$(PREFIX)/include
LIBDIR?=$(EXEC_PREFIX)/lib
APPDIR?=$(EXEC_PREFIX)/app

CFLAGS:=$(CFLAGS) -static -ffreestanding -Wall -Wextra -no-pie -fno-pic -fomit-frame-pointer -nostdlib
CPPFLAGS:=$(CPPFLAGS) -D__is_libc -Iinclude
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) $(DESTDIR)$(LIBDIR)/libc.a

ARCHDIR=../libc/arch/$(HOSTARCH)

OBJS=\
helloworld/hello.o

LINK_LIST=\
helloworld/hello.o \
$(LIBS) \

BINARIES=hello

.PHONY: all clean install
.SUFFIXES: .o .c

all: $(BINARIES)

hello: $(OBJS)
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f $(BINARIES)
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: $(BINARIES)
	mkdir -p $(DESTDIR)$(APPDIR)
	cp $(BINARIES) $(DESTDIR)$(APPDIR)