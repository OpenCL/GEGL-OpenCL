noinst_LTLIBRARIES = libmodule.la

MODULE_sources = 	\
    gegldatafiles.c	\
    geglmodule.c	\
    geglmoduledb.c	

MODULE_headers = 	\
    gegldatafiles.h	\
    geglmoduledb.h	\
    geglmodule.h	\
    geglmoduletypes.h

libmodule_la_SOURCES = $(MODULE_sources) $(MODULE_headers)
#libmoduleinclude_HEADERS = #$(MODULE_headers)
#libmoduleincludedir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl/module

INCLUDES = \
	   -I$(top_srcdir)	\
	   -I$(top_srcdir)/gegl	\
	   @DEP_CFLAGS@
