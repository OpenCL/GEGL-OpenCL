# General minimalistic compile file for self contained single
# file babl_extensions

#win32_libs = -lgw32c -liberty -lole32 -luuid -lwsock32
#no_undefined = -no-undefined

.PHONY: all clean distclean maintainer-clean mostlyclean dvi check install user-install

CC = gcc

CFILES  = $(wildcard *.c)
OFILES  = $(CFILES:.c=.o)
CFLAGS  += -g -O2 -Wall -fPIC
LDFLAGS += -lm
SOBJS   = affine.so
all: $(SOBJS)

CFLAGS  += -Wall -I. `pkg-config --cflags gegl`

affine.so: $(OFILES)
	$(CC) $(LDFLAGS) $(LDADD) $(OFILES) -o affine.so

%.o: %.c chant.h module.h matrix.h
	$(CC) $(CFLAGS) -c $<

#############################################################################
LDFLAGS += -shared

LDADD    = $(win32_libs) $(no-undefined) `pkg-config --libs gegl`
CFLAGS  += $(win32_libs) $(no-undefined)

clean:
	rm -f $(SOBJS) $(OFILES)
distclean:
	rm -f $(SOBJS) $(OFILES) Makefile
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
