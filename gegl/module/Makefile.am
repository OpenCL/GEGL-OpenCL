noinst_LTLIBRARIES = libmodule.la

MODULE_sources = 	\
    gegldatafiles.c	\
    geglmodule.c	\
    geglmoduledb.c	

MODULE_headers = 	\
    gegldatafiles.h	\
    geglmoduledb.h	\
    geglmodule.h

libmodule_la_SOURCES = $(MODULE_sources) $(MODULE_headers)
#libmoduleinclude_HEADERS = #$(MODULE_headers)
#libmoduleincludedir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl/module

INCLUDES = \
	   -I$(top_srcdir)			\
	   -I$(top_srcdir)/gegl			\
	   -I$(top_srcdir)/gegl/buffer		\
	   -I$(top_srcdir)/gegl/property-types	\
	   @DEP_CFLAGS@ @BABL_CFLAGS@
