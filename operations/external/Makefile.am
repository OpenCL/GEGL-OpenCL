if OS_WIN32
mwindows = -mwindows
no_undefined = -no-undefined
endif

INCLUDES = @DEP_CFLAGS@ @BABL_CFLAGS@ @CPPFLAGS@ \
           -I$(srcdir) -I$(top_srcdir)           \
           -I$(top_builddir)                     \
           -I$(top_srcdir)/gegl                  \
           -I$(top_srcdir)/gegl/buffer           \
           -I$(top_srcdir)/gegl/operation        \
           -I$(top_srcdir)/gegl/property-types   \
           -I$(top_srcdir)/gegl/module           \
           -I.
AM_LDFLAGS = -avoid-version -module $(no_undefined)
libgegl = $(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la

op_libs = @DEP_LIBS@ @BABL_LIBS@ $(libgegl)
### the stuff above this line could potentially be moved to a common file
### to reduce duplication

# start out with no ops, each if/endif block contains all the bits relevant
# for a specific dependency.
ops =

if HAVE_CAIRO
text_la_SOURCES = text.c
text_la_LIBADD = $(op_libs) $(CAIRO_LIBS) $(PANGO_LIBS)
text_la_CFLAGS = $(CAIRO_CFLAGS) $(PANGO_CFLAGS)
ops += text.la
endif

if HAVE_PNG
ops += png_load.la png_save.la
png_load_la_SOURCES = png-load.c
png_load_la_LIBADD = $(op_libs) $(PNG_LIBS)
png_load_la_CFLAGS = $(PNG_CFLAGS)

png_save_la_SOURCES = png-save.c
png_save_la_LIBADD = $(op_libs) $(PNG_LIBS)
png_save_la_CFLAGS = $(PNG_CFLAGS)
endif

if HAVE_JPEG
ops += jpg_load.la
jpg_load_la_SOURCES = jpg-load.c
jpg_load_la_LIBADD = $(op_libs) $(LIBJPEG)
endif


if HAVE_RSVG
ops += svg_load.la
svg_load_la_SOURCES = svg-load.c
svg_load_la_LIBADD = $(op_libs) $(CAIRO_LIBS) $(RSVG_LIBS)
svg_load_la_CFLAGS = $(CAIRO_CFLAGS) $(RSVG_CFLAGS)
endif

if HAVE_GTK
ops += pixbuf.la save_pixbuf.la
pixbuf_la_SOURCES      = pixbuf.c
pixbuf_la_LIBADD       = $(op_libs) $(CAIRO_LIBS) $(RSVG_LIBS)
pixbuf_la_CFLAGS       = $(CAIRO_CFLAGS) $(RSVG_CFLAGS)

save_pixbuf_la_SOURCES = save-pixbuf.c
save_pixbuf_la_LIBADD  = $(op_libs) $(CAIRO_LIBS) $(RSVG_LIBS)
save_pixbuf_la_CFLAGS  = $(CAIRO_CFLAGS) $(RSVG_CFLAGS)
endif

if HAVE_OPENEXR
ops += exr_load.la
exr_load_la_SOURCES = exr-load.cpp
exr_load_la_LIBADD = $(op_libs) $(OPENEXR_LIBS)
exr_load_la_CXXFLAGS = $(OPENEXR_CFLAGS)
endif

if HAVE_SDL
ops += display.la
display_la_SOURCES = display.c
display_la_LIBADD = $(op_libs) $(SDL_LIBS)
display_la_CFLAGS = $(SDL_CFLAGS)
endif

if HAVE_AVCODEC
if HAVE_AVFORMAT
ops += ff_load.la
ff_load_la_SOURCES = ff-load.c
ff_load_la_LIBADD = $(op_libs) $(AVCODEC_LIBS) $(AVFORMAT_LIBS)
ff_load_la_CFLAGS = $(AVCODEC_CFLAGS) $(AVFORMAT_CFLAGS)
endif
endif

opdir = $(DESTDIR)$(libdir)/gegl-@GEGL_API_VERSION@
op_LTLIBRARIES = $(ops)
