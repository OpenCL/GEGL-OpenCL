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
	gegl-config.c			\
	gegl-cpuaccel.c			\
	gegl-dot.c			\
	gegl-init.c			\
	gegl-instrument.c		\
	gegl-utils.c			\
	gegl-matrix.c			\
	gegl-xml.c			\
	\
	gegl-chant.h			\
	gegl-config.h			\
	gegl-cpuaccel.h			\
	gegl-debug.h			\
	gegl-dot.h			\
	gegl-init.h			\
	gegl-instrument.h		\
	gegl-module.h			\
	gegl-matrix.h			\
	gegl-plugin.h			\
	gegl-types-internal.h		\
	gegl-xml.h

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
	$(no_undefined) -export-dynamic -version-info $(GEGL_LIBRARY_VERSION)

INCLUDES = \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/gegl/buffer	\
	-I$(top_srcdir)/gegl/property-types	\
	-I$(top_srcdir)/gegl/operation\
	@DEP_CFLAGS@ @BABL_CFLAGS@

AM_CPPFLAGS = \
	-DLIBDIR=\""$(libdir)"\"	\
	-DGEGL_LOCALEDIR=\""$(GEGL_LOCALEDIR)"\" 

libgegl_@GEGL_API_VERSION@includedir = $(includedir)/gegl-$(GEGL_API_VERSION)

libgegl_@GEGL_API_VERSION@include_HEADERS =	\
    gegl.h				\
    gegl-types.h			\
    gegl-utils.h			\
    gegl-matrix.h			\
    gegl-chant.h			\
    gegl-simd.h				\
    gegl-plugin.h			\
    gegl-version.h			\
    buffer/gegl-buffer.h		\
    property-types/gegl-paramspecs.h	\
    property-types/gegl-color.h		\
    property-types/gegl-path.h		\
    property-types/gegl-curve.h
