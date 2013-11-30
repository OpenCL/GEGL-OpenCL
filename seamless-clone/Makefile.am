include $(top_srcdir)/operations/Makefile-common.am

#################################################
# A shared library for the seamless cloning API #
#################################################

GEGL_SC_publicdir = $(includedir)/gegl-$(GEGL_API_VERSION)/sc

GEGL_SC_public_HEADERS = \
	sc-common.h	\
	sc-context.h	\
	sc-outline.h	\
	sc-sample.h

GEGL_SC_SOURCES = \
	$(GEGL_SC_public_HEADERS)	\
	sc-context-private.h		\
	sc-context.c			\
	sc-outline.c			\
	sc-sample.c

libgegl_sc_@GEGL_API_VERSION@_la_SOURCES = \
	$(GEGL_SC_public_HEADERS)	\
	$(GEGL_SC_SOURCES)

libgegl_sc_@GEGL_API_VERSION@_la_LDFLAGS = \
	-avoid-version -export-dynamic $(no_undefined) $(P2TC_LIBS)

libgegl_sc_@GEGL_API_VERSION@_la_CFLAGS = \
  $(AM_CFLAGS) $(P2TC_CFLAGS)

lib_LTLIBRARIES = libgegl-sc-@GEGL_API_VERSION@.la

libgegl_sc_@GEGL_API_VERSION@_la_LIBADD = \
	$(libgegl) $(BABL_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gegl-sc-$(GEGL_API_VERSION).pc

