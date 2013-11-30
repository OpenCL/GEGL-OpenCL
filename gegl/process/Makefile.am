AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/graph \
	-I$(top_srcdir)/gegl/graph \
	-I$(top_builddir)/gegl/operation \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl \
	-I$(top_builddir)/gegl/process \
	-I$(top_srcdir)/gegl/process \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS)

noinst_LTLIBRARIES = libprocess.la

#libprocess_publicdir = $(includedir)/gegl-$(GEGL_API_VERSION)/process
#libprocess_public_HEADERS = #

libprocess_la_SOURCES = \
	gegl-eval-manager.c		\
	gegl-graph-traversal.c		\
	gegl-graph-traversal-debug.c	\
	gegl-list-visitor.c		\
	gegl-processor.c		\
	\
	gegl-eval-manager.h		\
	gegl-graph-debug.h		\
	gegl-graph-traversal.h		\
	gegl-graph-traversal-private.h	\
	gegl-list-visitor.h		\
	gegl-processor.h		\
	gegl-processor-private.h

#libprocess_la_SOURCES = $(lib_process_sources) $(libprocess_public_HEADERS)
