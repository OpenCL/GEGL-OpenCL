noinst_LTLIBRARIES = libpropertytypes.la

PROPERTYTYPES_sources = \
	gegl-color.c		\
	gegl-curve.c		\
	gegl-paramspecs.c


PROPERTYTYPES_headers = \
	gegl-color.h		\
	gegl-curve.h		\
	gegl-paramspecs.h


libpropertytypes_la_SOURCES = $(PROPERTYTYPES_sources) $(PROPERTYTYPES_headers)
libpropertytypesinclude_HEADERS = gegl-paramspecs.h #$(PROPERTYTYPES_headers)

libpropertytypesincludedir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl/property-types

INCLUDES = \
	   -I$(top_srcdir)	\
	   -I$(top_srcdir)/gegl	\
	   @DEP_CFLAGS@ @BABL_CFLAGS@

