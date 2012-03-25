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

libcl_publicdir = $(includedir)/gegl-$(GEGL_API_VERSION)/opencl

libcl_public_HEADERS = \
	gegl-cl.h \
	gegl-cl-init.h \
	gegl-cl-types.h \
	gegl-cl-color.h \
	cl_d3d10.h \
	cl_ext.h \
	cl_gl_ext.h \
	cl_gl.h \
	cl.h \
	cl_platform.h \
	opencl.h

libcl_sources = \
	gegl-cl-types.h \
	gegl-cl-init.c \
	gegl-cl-init.h \
	gegl-cl-color-kernel.h \
	gegl-cl-color.c \
	gegl-cl-color.h

noinst_LTLIBRARIES = libcl.la

libcl_la_SOURCES = $(libcl_sources) $(libcl_public_HEADERS)
