if PLATFORM_WIN32
no_undefined = -no-undefined
endif

SUBDIRS = buffer graph module operation process property-types

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types \
	-I$(top_builddir)/gegl/operation \
	-I$(top_srcdir)/gegl/operation \
	-DLIBDIR=\""$(libdir)"\" \
	-DGEGL_LOCALEDIR=\""$(GEGL_LOCALEDIR)"\"

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS)

AM_LDFLAGS = \
	$(no_undefined) -export-dynamic -version-info $(GEGL_LIBRARY_VERSION)

LIBS = \
	$(DEP_LIBS) $(BABL_LIBS)

GEGL_publicdir = $(includedir)/gegl-$(GEGL_API_VERSION)

GEGL_public_HEADERS =	\
    gegl.h				\
    gegl-types.h			\
    gegl-utils.h			\
    gegl-matrix.h			\
    gegl-chant.h			\
    gegl-lookup.h			\
    gegl-simd.h				\
    gegl-plugin.h			\
    gegl-version.h			\
    buffer/gegl-buffer.h		\
    property-types/gegl-paramspecs.h	\
    property-types/gegl-color.h		\
    property-types/gegl-path.h		\
    property-types/gegl-curve.h

GEGL_sources = \
	gegl-c.c			\
	gegl-config.c			\
	gegl-cpuaccel.c			\
	gegl-dot.c			\
	gegl-dot-visitor.c		\
	gegl-init.c			\
	gegl-instrument.c		\
	gegl-utils.c			\
	gegl-lookup.c			\
	gegl-matrix.c			\
	gegl-xml.c			\
	\
	gegl-chant.h			\
	gegl-lookup.h			\
	gegl-config.h			\
	gegl-cpuaccel.h			\
	gegl-debug.h			\
	gegl-dot.h			\
	gegl-dot-visitor.h		\
	gegl-init.h			\
	gegl-instrument.h		\
	gegl-module.h			\
	gegl-matrix.h			\
	gegl-plugin.h			\
	gegl-types-internal.h		\
	gegl-xml.h

lib_LTLIBRARIES = libgegl-@GEGL_API_VERSION@.la

libgegl_@GEGL_API_VERSION@_la_SOURCES = $(GEGL_sources) $(GEGL_public_HEADERS)
libgegl_@GEGL_API_VERSION@_la_LIBADD = \
	$(top_builddir)/gegl/buffer/libbuffer.la		\
	$(top_builddir)/gegl/module/libmodule.la		\
	$(top_builddir)/gegl/operation/liboperation.la		\
	$(top_builddir)/gegl/graph/libgraph.la			\
	$(top_builddir)/gegl/process/libprocess.la		\
	$(top_builddir)/gegl/property-types/libpropertytypes.la

