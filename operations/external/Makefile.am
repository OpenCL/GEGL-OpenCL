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
ops += path.la vector-fill.la vector-stroke.la
path_la_SOURCES = path.c
path_la_LIBADD = $(op_libs) $(CAIRO_LIBS)
path_la_CFLAGS = $(AM_CFLAGS) $(CAIRO_CFLAGS)
vector_fill_la_SOURCES = vector-fill.c
vector_fill_la_LIBADD = $(op_libs) $(CAIRO_LIBS)
vector_fill_la_CFLAGS = $(AM_CFLAGS) $(CAIRO_CFLAGS)
vector_stroke_la_SOURCES = vector-stroke.c
vector_stroke_la_LIBADD = $(op_libs) $(CAIRO_LIBS)
vector_stroke_la_CFLAGS = $(AM_CFLAGS) $(CAIRO_CFLAGS)
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
ops += jpg-load.la jpg-save.la
jpg_load_la_SOURCES = jpg-load.c
jpg_load_la_LIBADD = $(op_libs) $(LIBJPEG)

jpg_save_la_SOURCES = jpg-save.c
jpg_save_la_LIBADD = $(op_libs) $(LIBJPEG)
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

ops += exr-save.la
exr_save_la_SOURCES = exr-save.cc
exr_save_la_LIBADD = $(op_libs) $(OPENEXR_LIBS)
exr_save_la_CXXFLAGS = $(AM_CFLAGS) $(OPENEXR_CFLAGS)
endif

if HAVE_SDL
ops += sdl-display.la
sdl_display_la_LIBADD = $(op_libs) $(SDL_LIBS)
sdl_display_la_CFLAGS = $(AM_CFLAGS) $(SDL_CFLAGS)
endif

if HAVE_LIBRAW
ops += raw-load.la
raw_load_la_SOURCES = raw-load.c
raw_load_la_LIBADD = $(op_libs) $(LIBRAW_LIBS)
raw_load_la_CFLAGS = $(AM_CFLAGS) $(LIBRAW_CFLAGS)
endif

if HAVE_V4L
ops += v4l.la
v4l_la_SOURCES = v4l.c
v4l_la_LIBADD = $(op_libs)
endif

EXTRA_DIST=v4lutils/v4lutils.c v4lutils/v4lutils.h

if HAVE_JASPER
ops += jp2-load.la
jp2_load_la_SOURCES = jp2-load.c
jp2_load_la_LIBADD  = $(op_libs) $(JASPER_LIBS)
jp2_load_la_CFLAGS  = $(AM_CFLAGS) $(JASPER_CFLAGS)
endif

if HAVE_AVFORMAT
ops += ff-load.la ff-save.la
ff_load_la_SOURCES = ff-load.c
ff_load_la_LIBADD = $(op_libs) $(AVFORMAT_LIBS)
ff_load_la_CFLAGS = $(AM_CFLAGS) $(AVFORMAT_CFLAGS) 

ff_save_la_SOURCES = ff-save.c
ff_save_la_LIBADD = $(op_libs) $(AVFORMAT_LIBS)
ff_save_la_CFLAGS = $(AM_CFLAGS) $(AVFORMAT_CFLAGS) -Wno-deprecated-declarations
endif

if HAVE_UMFPACK
ops += matting-levin.la
matting_levin_la_SOURCES = matting-levin.c matting-levin-cblas.c matting-levin-cblas.h
matting_levin_la_LIBADD  = $(op_libs) $(UMFPACK_LIBS)
matting_levin_la_CFLAGS  = $(AM_CFLAGS)
endif

if HAVE_LCMS
ops += lcms-from-profile.la
lcms_from_profile_la_SOURCES = lcms-from-profile.c
lcms_from_profile_la_LIBADD  = $(op_libs) $(LCMS_LIBS)
lcms_from_profile_la_CFLAGS  = $(AM_CFLAGS)
endif
 
if HAVE_TIFF
ops += tiff-load.la
tiff_load_la_SOURCES = tiff-load.c
tiff_load_la_LIBADD = $(op_libs) $(TIFF_LIBS)
tiff_load_la_CFLAGS = $(AM_CFLAGS) $(TIFF_CFLAGS)

ops += tiff-save.la
tiff_save_la_SOURCES = tiff-save.c
tiff_save_la_LIBADD = $(op_libs) $(TIFF_LIBS)
tiff_save_la_CFLAGS = $(AM_CFLAGS) $(TIFF_CFLAGS)
endif

if HAVE_WEBP
ops += webp-load.la
webp_load_la_SOURCES = webp-load.c
webp_load_la_LIBADD  = $(op_libs) $(WEBP_LIBS)
webp_load_la_CFLAGS  = $(AM_CFLAGS) $(WEBP_CFLAGS)

ops += webp-save.la
webp_save_la_SOURCES = webp-save.c
webp_save_la_LIBADD  = $(op_libs) $(WEBP_LIBS)
webp_save_la_CFLAGS  = $(AM_CFLAGS) $(WEBP_CFLAGS)
endif

# No dependencies
ops += ppm-load.la ppm-save.la
ppm_load_la_SOURCES = ppm-load.c
ppm_load_la_LIBADD = $(op_libs)
ppm_save_la_SOURCES = ppm-save.c
ppm_save_la_LIBADD = $(op_libs)

# No dependencies
ops += npy-save.la
npy_save_la_SOURCES = npy-save.c
npy_save_la_LIBADD = $(op_libs)

# Dependencies are in our source tree
ops += rgbe-load.la rgbe-save.la
rgbe_load_la_SOURCES = rgbe-load.c
rgbe_load_la_CFLAGS = $(AM_CFLAGS) -I $(top_srcdir)/libs
rgbe_load_la_LIBADD = $(op_libs) $(top_builddir)/libs/rgbe/librgbe.la
rgbe_save_la_SOURCES = rgbe-save.c
rgbe_save_la_CFLAGS = $(AM_CFLAGS) -I $(top_srcdir)/libs
rgbe_save_la_LIBADD = $(op_libs) $(top_builddir)/libs/rgbe/librgbe.la

# Dependencies are in our source tree
if HAVE_CAIRO
ops += npd.la
npd_la_SOURCES = npd.c
npd_la_CFLAGS = $(AM_CFLAGS) $(NPD_CFLAGS) $(CAIRO_CFLAGS)
npd_la_LIBADD = $(op_libs) $(NPD_LIBS) $(CAIRO_LIBS)
endif

BUILT_SOURCES = $(subst .c,.c.h,$(wildcard $(srcdir)/*.c))
EXTRA_DIST += $(BUILT_SOURCES)

opdir = $(libdir)/gegl-@GEGL_API_VERSION@
op_LTLIBRARIES = $(ops)
