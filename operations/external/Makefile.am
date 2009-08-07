include $(top_srcdir)/operations/Makefile-common.am

# start out with no ops, each if/endif block contains all the bits relevant
# for a specific dependency.
ops =

if HAVE_PANGOCAIRO
ops += text.la
text_la_SOURCES = text.c
text_la_LIBADD = $(op_libs) $(PANGOCAIRO_LIBS)
text_la_CFLAGS = $(AM_CFLAGS) $(PANGOCAIRO_CFLAGS)
endif

if HAVE_CAIRO
ops += path.la
path_la_SOURCES = path.c
path_la_LIBADD = $(op_libs) $(CAIRO_LIBS)
path_la_CFLAGS = $(AM_CFLAGS) $(CAIRO_CFLAGS)
endif

if HAVE_PNG
ops += png-load.la png-save.la
png_load_la_SOURCES = png-load.c
png_load_la_LIBADD = $(op_libs) $(PNG_LIBS)
png_load_la_CFLAGS = $(AM_CFLAGS) $(PNG_CFLAGS)

png_save_la_SOURCES = png-save.c
png_save_la_LIBADD = $(op_libs) $(PNG_LIBS)
png_save_la_CFLAGS = $(AM_CFLAGS) $(PNG_CFLAGS)
endif

if HAVE_JPEG
ops += jpg-load.la
jpg_load_la_SOURCES = jpg-load.c
jpg_load_la_LIBADD = $(op_libs) $(LIBJPEG)
endif

if HAVE_CAIRO
if HAVE_RSVG
ops += svg-load.la
svg_load_la_SOURCES = svg-load.c
svg_load_la_LIBADD = $(op_libs) $(CAIRO_LIBS) $(RSVG_LIBS)
svg_load_la_CFLAGS = $(AM_CFLAGS) $(CAIRO_CFLAGS) $(RSVG_CFLAGS)
endif
endif

if HAVE_GDK_PIXBUF
ops += pixbuf.la save-pixbuf.la
pixbuf_la_SOURCES      = pixbuf.c
pixbuf_la_LIBADD       = $(op_libs) $(GDK_PIXBUF_LIBS)
pixbuf_la_CFLAGS       = $(AM_CFLAGS) $(GDK_PIXBUF_CFLAGS)

save_pixbuf_la_SOURCES = save-pixbuf.c
save_pixbuf_la_LIBADD  = $(op_libs) $(GDK_PIXBUF_LIBS)
save_pixbuf_la_CFLAGS  = $(AM_CFLAGS) $(GDK_PIXBUF_CFLAGS)
endif

if HAVE_OPENEXR
ops += exr-load.la
exr_load_la_SOURCES = exr-load.cpp
exr_load_la_LIBADD = $(op_libs) $(OPENEXR_LIBS)
exr_load_la_CXXFLAGS = $(AM_CFLAGS) $(OPENEXR_CFLAGS)
endif

if HAVE_SDL
ops += display.la
display_la_LIBADD = $(op_libs) $(SDL_LIBS)
display_la_CFLAGS = $(AM_CFLAGS) $(SDL_CFLAGS)
endif

if HAVE_OPENRAW
ops += openraw.la
openraw_la_SOURCES = openraw.c
openraw_la_LIBADD = $(op_libs) $(OPENRAW_LIBS)
openraw_la_CFLAGS = $(AM_CFLAGS) $(OPENRAW_CFLAGS)
endif

if HAVE_V4L
ops += v4l.la
v4l_la_SOURCES = v4l.c
v4l_la_LIBADD = $(op_libs)
endif

EXTRA_DIST=v4lutils/v4lutils.c v4lutils/v4lutils.h

if HAVE_AVFORMAT
ops += ff-load.la
ff_load_la_SOURCES = ff-load.c
ff_load_la_LIBADD = $(op_libs) $(AVFORMAT_LIBS)
ff_load_la_CFLAGS = $(AM_CFLAGS) $(AVFORMAT_CFLAGS)
endif

# No dependencies
ops += ppm-load.la ppm-save.la
ppm_load_la_SOURCES = ppm-load.c
ppm_load_la_LIBADD = $(op_libs)
ppm_save_la_SOURCES = ppm-save.c
ppm_save_la_LIBADD = $(op_libs)

opdir = $(libdir)/gegl-@GEGL_API_VERSION@
op_LTLIBRARIES = $(ops)
