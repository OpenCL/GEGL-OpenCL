
SUBDIRS = data

EXTRA_DIST = $(wildcard $(srcdir)/*.py))

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/operation \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_builddir)/gegl/module \
	-I$(top_srcdir)/gegl/module \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS)

AM_LDFLAGS =  \
	$(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la \
	$(BABL_LIBS) \
	$(DEP_LIBS)

bin_PROGRAMS = \
	2geglbuffer \
	csugar \
	float-lookup \
	gegl-convert \
	gegl-slicer \
	geglbuffer-add-image \
	geglbuffer-clock \
	hello-world

2geglbuffer_SOURCES = 2geglbuffer.c
csugar_SOURCES = csugar.c
float_lookup_SOURCES = float-lookup.c
gegl_convert_SOURCES = gegl-convert.c
gegl_slicer_SOURCES = gegl-slicer.c
geglbuffer_add_image_SOURCES = geglbuffer-add-image.c
geglbuffer_clock_SOURCES = geglbuffer-clock.c
hello_world_SOURCES = hello-world.c
