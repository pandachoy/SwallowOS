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
hello_proj/hellohellohello.o \
shell_proj/shell.o

LINK_LIST=\
hello_proj/hellohellohello.o \
shell_proj/shell.o \
$(LIBS) \

BINARIES=hello shell

.PHONY: all clean install
.SUFFIXES: .o .c

all: $(BINARIES)

hello: hello_proj/hellohellohello.o
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) hello_proj/hellohellohello.o $(LIBS)

shell: shell_proj/shell.o
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) shell_proj/shell.o $(LIBS)

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f $(BINARIES) || true
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: $(BINARIES)
	mkdir -p $(DESTDIR)$(APPDIR)
	cp $(BINARIES) $(DESTDIR)$(APPDIR)