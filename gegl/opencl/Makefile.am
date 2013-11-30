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

libcl_publicdir = $(includedir)/gegl-$(GEGL_API_VERSION)/opencl

libcl_public_HEADERS = \
	gegl-cl.h \
	gegl-cl-init.h \
	gegl-cl-types.h \
	gegl-cl-color.h \
	gegl-cl-random.h \
	cl_d3d10.h \
	cl_ext.h \
	cl_gl_ext.h \
	cl_gl.h \
	cl.h \
	cl_platform.h \
	opencl.h

libcl_sources = \
	gegl-cl.c \
	gegl-cl-types.h \
	gegl-cl-init.c \
	gegl-cl-init.h \
	gegl-cl-color.c \
	gegl-cl-color.h \
	gegl-cl-introspection-support.h \
	gegl-cl-random.c \
	gegl-cl-random.h

noinst_LTLIBRARIES = libcl.la

libcl_la_SOURCES = $(libcl_sources) $(libcl_public_HEADERS)
