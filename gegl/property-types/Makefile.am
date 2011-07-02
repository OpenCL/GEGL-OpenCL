AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS)

noinst_LTLIBRARIES = libpropertytypes.la

libpropertytypes_la_SOURCES = \
	gegl-color.c \
	gegl-curve.c \
	gegl-path.c \
	gegl-paramspecs.c \
	\
	gegl-color.h		\
	gegl-curve.h		\
	gegl-path.h		\
	gegl-paramspecs.h
