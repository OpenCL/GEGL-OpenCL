if OS_WIN32
no_undefined = -no-undefined
endif

SUBDIRS = buffer module operation graph process

libbuffer = $(top_builddir)/gegl/buffer/libbuffer.la
libmodule = $(top_builddir)/gegl/module/libmodule.la
liboperation = $(top_builddir)/gegl/operation/liboperation.la
libgraph = $(top_builddir)/gegl/graph/libgraph.la
libprocess = $(top_builddir)/gegl/process/libprocess.la

lib_LTLIBRARIES = libgegl-@GEGL_API_VERSION@.la

GEGL_sources = \
	gegl-color.c			\
	gegl-curve.c			\
	gegl-extension-handler.c	\
	gegl-init.c			\
	gegl-instrument.c		\
	gegl-dot.c			\
	gegl-paramspecs.c		\
	gegl-utils.c			\
	gegl-xml.c			\
	\
	gegl-color.h			\
	gegl-curve.h			\
	gegl-extension-handler.h	\
	gegl-init.h			\
	gegl-types.h			\
	gegl-dot.h			\
	gegl-xml.h			\
	gegl-plugin.h			\
	gegl-chant.h			\
	gegl-instrument.h		\
	gegl-module.h

GEGL_public_headers = gegl-paramspecs.h	\
	gegl-utils.h			

libgegl_@GEGL_API_VERSION@_la_SOURCES = $(GEGL_sources) $(GEGL_public_headers)

libgegl_@GEGL_API_VERSION@_la_LIBADD =	\
	$(libbuffer) 	\
	$(libmodule) 	\
	$(liboperation) \
	$(libgraph) 	\
	$(libprocess)

libgegl_@GEGL_API_VERSION@_la_LDFLAGS = \
	@DEP_LIBS@ @BABL_LIBS@	\
	$(no_undefined) -version-info $(GEGL_LIBRARY_VERSION)

INCLUDES = \
	-I$(top_srcdir)	\
	@DEP_CFLAGS@ @BABL_CFLAGS@

AM_CPPFLAGS = -DPREFIX=\""$(prefix)"\"

libgegl_@GEGL_API_VERSION@includedir = $(includedir)/gegl-$(GEGL_API_VERSION)
libgegl_@GEGL_API_VERSION@subincludedir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl

libgegl_@GEGL_API_VERSION@include_HEADERS = gegl.h
libgegl_@GEGL_API_VERSION@subinclude_HEADERS = $(GEGL_public_headers) 
