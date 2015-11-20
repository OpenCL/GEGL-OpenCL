include $(top_srcdir)/operations/Makefile-common.am

# start out with no ops, each if/endif block contains all the bits relevant
# for a specific dependency.
ops =

if HAVE_CAIRO
line_profile_la_SOURCES = line-profile.c
line_profile_la_LIBADD = $(op_libs) $(CAIRO_LIBS) $(PANGO_LIBS)
line_profile_la_CFLAGS = $(CAIRO_CFLAGS) $(PANGO_CFLAGS) $(BABL_CFLAGS) $(GLIB_CFLAGS)
ops += line-profile.la
endif

if HAVE_LUA
ops += gluas.la
gluas_la_SOURCES = gluas.c
gluas_la_LIBADD  = $(op_libs) $(LUA_LIBS)
gluas_la_CFLAGS  = $(LUA_CFLAGS) $(BABL_CFLAGS) $(GLIB_CFLAGS)
endif

if HAVE_LENSFUN
ops += lens-correct.la
lens_correct_la_SOURCES = lens-correct.c
lens_correct_la_LIBADD = $(op_libs) $(LENSFUN_LIBS)
lens_correct_la_CFLAGS = $(AM_CFLAGS) $(LENSFUN_CFLAGS)
endif

if HAVE_V4L2
ops += v4l2.la
v4l2_la_SOURCES = v4l2.c
v4l2_la_LIBADD = $(op_libs) $(V4L2_LIBS)
v4l2_la_CFLAGS = $(AM_CFLAGS) $(V4L2_CFLAGS)
endif

EXTRA_DIST = $(wildcard $(srcdir)/*.h)
BUILT_SOURCES = $(subst .c,.c.h,$(wildcard $(srcdir)/*.c))

opdir = $(libdir)/gegl-@GEGL_API_VERSION@
op_LTLIBRARIES = $(ops)
