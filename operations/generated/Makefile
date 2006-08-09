# General minimalistic compile file for self contained single
# file babl_extensions

#win32_libs = -lgw32c -liberty -lole32 -luuid -lwsock32
#no_undefined = -no-undefined

CC = gcc

CFILES  = $(wildcard *.c)
CFLAGS  += -g -O2 -Wall
SOBJS   = $(CFILES:.c=.so)
all: $(SOBJS)

CFLAGS  += -Wall -I. `pkg-config --cflags gegl`

%.so: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD)

LDFLAGS += -shared 

LDADD    = $(win32_libs) $(no-undefined) `pkg-config --libs gegl`
CFLAGS  += $(win32_libs) $(no-undefined) `pkg-config --cflags gegl`

clean:
	rm -f $(SOBJS) *~
distclean:
	rm -f $(SOBJS) Makefile
maintainer-clean: distclean
mostlyclean:

srcdir       = .
top_builddir = ..
INSTALL      = /usr/bin/install -c
SHELL        = /bin/sh
ext_dir      = /usr/local/lib/gegl

install: user-install

user-install: all
	$(INSTALL) -d $(ext_dir)
	$(INSTALL) $(SOBJS)  $(ext_dir)
distdir:
	cp $(CFILES) $(Makefile.in) $$distdir

dvi:
	#playing along with automake
check: all
