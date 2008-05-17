#include $(top_srcdir)/operations/Makefile-common.am

SUBDIRS = data
CFILES = $(wildcard $(srcdir)/*.c)
bins   = $(subst $(srcdir)/,,$(CFILES:.c=))
EXTRA_DIST = $(wildcard *.c)

all-local: $(bins)

clean-local:
	rm -f *.lo $(bins)

INCLUDES = \
	-I$(top_builddir) \
	-I$(top_srcdir)	-I$(top_srcdir)/gegl -I$(top_srcdir)/gegl/buffer \
	-I$(top_srcdir)/gegl/property-types \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_srcdir)/gegl/module \
	@DEP_CFLAGS@ @GTK_CFLAGS@ @BABL_CFLAGS@ @PNG_CFLAGS@

AM_LDFLAGS =  \
	../gegl/libgegl-@GEGL_API_VERSION@.la \
	@DEP_LIBS@ @GTK_LIBS@ @BABL_LIBS@ @PNG_LIBS@

%: %.c $(GEGLHEADERS)
	$(LIBTOOL) --mode=compile $(CC) $< -c -o $@.lo $(INCLUDES) -DHAVE_CONFIG_H
	$(LIBTOOL) --mode=link    $(CC) $@.lo -o $@ $(AM_LDFLAGS)
