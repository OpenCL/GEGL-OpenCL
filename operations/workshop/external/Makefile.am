if OS_WIN32
mwindows = -mwindows
no_undefined = -no-undefined
libgegl = $(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la
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

op_libs = @DEP_LIBS@ @BABL_LIBS@ $(libgegl)
### the stuff above this line could potentially be moved to a common file
### to reduce duplication

# start out with no ops, each if/endif block contains all the bits relevant
# for a specific dependency.
ops =

if HAVE_CAIRO
line_profile_la_SOURCES = line-profile.c
line_profile_la_LIBADD = $(op_libs) $(CAIRO_LIBS) $(PANGO_LIBS)
line_profile_la_CFLAGS = $(CAIRO_CFLAGS) $(PANGO_CFLAGS)
ops += line_profile.la
endif

if HAVE_GTK
ops += gtk_display.la
gtk_display_la_SOURCES = gtk-display.c
gtk_display_la_LIBADD  = $(op_libs) $(GTK_LIBS)
gtk_display_la_CFLAGS  = $(GTK_CFLAGS)
endif

if HAVE_AVCODEC
if HAVE_AVFORMAT
ops += ff_save.la
ff_save_la_SOURCES = ff-save.c
ff_save_la_LIBADD = $(op_libs) $(AVCODEC_LIBS) $(AVFORMAT_LIBS)
ff_save_la_CFLAGS = $(AVCODEC_CFLAGS) $(AVFORMAT_CFLAGS)
endif
endif

if HAVE_LUA
ops += gluas.la
gluas_la_SOURCES = gluas.c
gluas_la_LIBADD  = $(op_libs) $(LUA_LIBS)
gluas_la_CFLAGS  = $(LUA_CFLAGS)
endif

opdir = $(DESTDIR)$(libdir)/gegl-@GEGL_API_VERSION@
op_LTLIBRARIES = $(ops)
