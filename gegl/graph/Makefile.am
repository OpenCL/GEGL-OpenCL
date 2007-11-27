noinst_LTLIBRARIES = libgraph.la

GRAPH_sources = \
	gegl-connection.c	\
	gegl-node.c		\
	gegl-node-dynamic.c	\
	gegl-pad.c		\
	gegl-visitor.c		\
	gegl-visitable.c


GRAPH_headers = \
	gegl-connection.h	\
	gegl-node.h		\
	gegl-node-dynamic.h	\
	gegl-pad.h		\
	gegl-visitor.h		\
	gegl-visitable.h


libgraph_la_SOURCES = $(GRAPH_sources) $(GRAPH_headers)
#libgraphinclude_HEADERS = #
#libgraphincludedir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl/graph

INCLUDES = \
	   -I$(top_srcdir)	\
	   -I$(top_srcdir)/gegl	\
	   @DEP_CFLAGS@ @BABL_CFLAGS@

