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

noinst_LTLIBRARIES = libprocess.la

#libprocess_publicdir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl/process
#libprocess_public_HEADERS = #

libprocess_la_SOURCES = \
	gegl-need-visitor.c		\
	gegl-debug-rect-visitor.c	\
	gegl-eval-mgr.c			\
	gegl-eval-visitor.c		\
	gegl-finish-visitor.c		\
	gegl-have-visitor.c		\
	gegl-prepare-visitor.c		\
	gegl-processor.c		\
	\
	gegl-need-visitor.h		\
	gegl-debug-rect-visitor.h	\
	gegl-eval-mgr.h			\
	gegl-eval-visitor.h		\
	gegl-finish-visitor.h		\
	gegl-have-visitor.h		\
	gegl-prepare-visitor.h		\
	gegl-processor.h

#libprocess_la_SOURCES = $(lib_process_sources) $(libprocess_public_HEADERS)

