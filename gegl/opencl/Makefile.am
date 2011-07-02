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

LIBS = $(OPENCL_LIBS)

noinst_LTLIBRARIES = libcl.la

#libcl_publicdir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl/cl
#libcl_public_HEADERS = gegl-cl-init.h

libcl_la_SOURCES = \
	gegl-cl-init.c \
	gegl-cl-init.h \
	gegl-cl-types.h

#libcl_la_SOURCES = $(libcl_sources) $(libcl_public_HEADERS)
