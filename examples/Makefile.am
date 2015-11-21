
SUBDIRS = data

EXTRA_DIST = $(wildcard $(srcdir)/*.py)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/graph \
	-I$(top_srcdir)/gegl/graph \
	-I$(top_builddir)/gegl/module \
	-I$(top_srcdir)/gegl/module \
	-I$(top_builddir)/gegl/operation \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl \
	-I$(top_builddir)/gegl/process \
	-I$(top_srcdir)/gegl/process \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS) $(GEXIV2_CFLAGS) $(SDL_CFLAGS)

AM_LDFLAGS =  \
	$(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la \
	$(GEXIV2_LIBS) \
	$(BABL_LIBS) \
	$(DEP_LIBS) \
	$(SDL_LIBS) \
	$(MATH_LIB)

bin_PROGRAMS = \
	2geglbuffer \
	gegl-convert \
	gegl-slicer \
	video-invert \
	geglbuffer-add-image \
	geglbuffer-clock \
	frame-counter \
	hello-world

frame_counter_SOURCES = frame-counter.c
2geglbuffer_SOURCES = 2geglbuffer.c
gegl_convert_SOURCES = gegl-convert.c
gegl_slicer_SOURCES = gegl-slicer.c
video_invert_SOURCES = video-invert.c
geglbuffer_add_image_SOURCES = geglbuffer-add-image.c
geglbuffer_clock_SOURCES = geglbuffer-clock.c
hello_world_SOURCES = hello-world.c

if HAVE_SDL
bin_PROGRAMS += sdl-draw  gegl-video
sdl_draw_SOURCES = sdl-draw.c
sdl_draw_LDADD = $(SDL_LIBS)
sdl_draw_CFLAGS = $(AM_CFLAGS) $(SDL_CFLAGS)
endif

