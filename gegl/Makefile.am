if OS_WIN32
no_undefined = -no-undefined
endif

SUBDIRS = buffer graph module operation process property-types

libbuffer = $(top_builddir)/gegl/buffer/libbuffer.la
libmodule = $(top_builddir)/gegl/module/libmodule.la
liboperation = $(top_builddir)/gegl/operation/liboperation.la
libgraph = $(top_builddir)/gegl/graph/libgraph.la
libprocess = $(top_builddir)/gegl/process/libprocess.la
libpropertytypes = $(top_builddir)/gegl/property-types/libpropertytypes.la

lib_LTLIBRARIES = libgegl-@GEGL_API_VERSION@.la

GEGL_sources = \
	gegl-init.c			\
	gegl-instrument.c		\
	gegl-dot.c			\
	gegl-utils.c			\
	gegl-xml.c			\
	\
	gegl-init.h			\
	gegl-types.h			\
	gegl-dot.h			\
	gegl-xml.h			\
	gegl-plugin.h			\
	gegl-chant.h			\
	gegl-instrument.h		\
	gegl-module.h

libgegl_@GEGL_API_VERSION@_la_SOURCES = $(GEGL_sources) $(GEGL_public_headers)

libgegl_@GEGL_API_VERSION@_la_LIBADD =	\
	$(libbuffer) 	\
	$(libmodule) 	\
	$(liboperation) \
	$(libgraph) 	\
	$(libprocess)	\
	$(libpropertytypes)

libgegl_@GEGL_API_VERSION@_la_LDFLAGS = \
	@DEP_LIBS@ @BABL_LIBS@	\
	$(no_undefined) -version-info $(GEGL_LIBRARY_VERSION)

INCLUDES = \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/gegl/buffer	\
	@DEP_CFLAGS@ @BABL_CFLAGS@

AM_CPPFLAGS = -DLIBDIR=\""$(libdir)"\"

libgegl_@GEGL_API_VERSION@includedir = $(includedir)/gegl-$(GEGL_API_VERSION)

libgegl_@GEGL_API_VERSION@include_HEADERS =	\
    gegl.h				\
    gegl-utils.h			\
    gegl-chant.h			\
    buffer/gegl-buffer.h		\
    module/geglmoduletypes.h		\
    module/geglmoduledb.h		\
    module/geglmodule.h			\
    property-types/gegl-paramspecs.h
