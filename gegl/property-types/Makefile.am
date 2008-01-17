noinst_LTLIBRARIES = libpropertytypes.la

PROPERTYTYPES_sources = \
	gegl-color.c		\
	gegl-curve.c		\
	gegl-vector.c		\
	gegl-paramspecs.c


PROPERTYTYPES_headers = \
	gegl-color.h		\
	gegl-curve.h		\
	gegl-vector.h		\
	gegl-paramspecs.h


libpropertytypes_la_SOURCES = $(PROPERTYTYPES_sources) $(PROPERTYTYPES_headers)

INCLUDES = \
	   -I$(top_srcdir)	\
	   -I$(top_srcdir)/gegl	\
	   @DEP_CFLAGS@ @BABL_CFLAGS@

