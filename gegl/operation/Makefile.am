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

liboperation_publicdir = $(includedir)/gegl-$(GEGL_API_VERSION)/operation

liboperation_public_HEADERS = \
	gegl-extension-handler.h         \
	gegl-operation.h        	 \
	gegl-operation-area-filter.h     \
	gegl-operation-composer.h     	 \
	gegl-operation-composer3.h     	 \
	gegl-operation-context.h         \
	gegl-operation-filter.h       	 \
	gegl-operation-meta.h       	 \
	gegl-operation-meta-json.h       \
	gegl-operation-point-composer.h  \
	gegl-operation-point-composer3.h \
	gegl-operation-point-filter.h    \
	gegl-operation-point-render.h    \
	gegl-operation-property-keys.h \
	gegl-operation-sink.h       	 \
	gegl-operation-source.h          \
	gegl-operation-temporal.h

liboperation_sources = \
	gegl-extension-handler.c		\
	gegl-extension-handler-private.h \
	gegl-operation.c			\
	gegl-operation-area-filter.c		\
	gegl-operation-composer.c		\
	gegl-operation-composer3.c		\
	gegl-operation-filter.c			\
	gegl-operation-meta.c			\
	gegl-operation-meta-json.c			\
	gegl-operation-point-composer.c		\
	gegl-operation-point-composer3.c	\
	gegl-operation-point-filter.c		\
	gegl-operation-point-render.c		\
	gegl-operation-property-keys.c \
	gegl-operation-sink.c			\
	gegl-operation-source.c			\
	gegl-operation-temporal.c		\
	gegl-operation-context.c		\
	gegl-operation-context-private.h \
	gegl-operations.c			\
	gegl-operations.h

noinst_LTLIBRARIES = liboperation.la

liboperation_la_SOURCES = $(liboperation_public_HEADERS) $(liboperation_sources)

