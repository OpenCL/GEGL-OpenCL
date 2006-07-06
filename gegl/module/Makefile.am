noinst_LTLIBRARIES = libmodule.la

BUFFER_sources = 	\
    gegldatafiles.c	\
    geglmodule.c	\
    geglmoduledb.c	

BUFFER_headers = 	\
    gegldatafiles.h	\
    geglmoduledb.h	\
    geglmodule.h	\
    geglmoduletypes.h

libmodule_la_SOURCES = $(BUFFER_sources) $(BUFFER_headers)
libmoduleinclude_HEADERS = $(BUFFER_headers)

libmoduleincludedir = $(includedir)/gegl-1.0/gegl/module

INCLUDES = \
	   -I$(top_srcdir)	\
	   -I$(top_srcdir)/gegl	\
	   @DEP_CFLAGS@
