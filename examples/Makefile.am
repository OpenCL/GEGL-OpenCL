#include $(top_srcdir)/operations/Makefile-common.am

SUBDIRS = data
CFILES = $(wildcard $(srcdir)/*.c)
bins   = $(subst $(srcdir)/,,$(CFILES:.c=))
EXTRA_DIST = $(wildcard *.c) $(wildcard util/*.[ch])

all-local: $(bins)

%: %.c $(GEGLHEADERS)
	$(LIBTOOL) --mode=compile $(CC) $(DEFS) -I. -I$(top_builddir) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -c -o $@.lo $<
	$(LIBTOOL) --mode=link    $(CC) $(AM_LDFLAGS) $(LDFLAGS) $(CFLAGS) -o $@ $@.lo $(LIBS)

clean-local:
	rm -f *.lo $(bins)

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/operation \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_builddir)/gegl/module \
	-I$(top_srcdir)/gegl/module

AM_CFLAGS = $(DEP_CFLAGS) $(GTK_CFLAGS) $(BABL_CFLAGS) $(PNG_CFLAGS)

AM_LDFLAGS =  \
	$(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la \
	$(DEP_LIBS) $(GTK_LIBS) $(BABL_LIBS) $(PNG_LIBS)
