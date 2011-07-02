AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS)

noinst_LTLIBRARIES = libgraph.la

#libgraph_publicdir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl/graph
#libgraph_public_HEADERS = #

libgraph_la_SOURCES = \
	gegl-connection.c	\
	gegl-node.c		\
	gegl-pad.c		\
	gegl-visitor.c		\
	gegl-visitable.c	\
	\
	gegl-connection.h	\
	gegl-node.h		\
	gegl-pad.h		\
	gegl-visitor.h		\
	gegl-visitable.h

#libgraph_la_SOURCES = $(libgraph_sources) $(libgraph_public_HEADERS)

