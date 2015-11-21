if OS_WIN32
no_undefined = -no-undefined
endif

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

AM_CFLAGS = \
	$(DEP_CFLAGS) $(BABL_CFLAGS) $(PNG_CFLAGS) \
        $(MRG_CFLAGS) $(GEXIV2_CFLAGS)

AM_LDFLAGS =  \
	$(no_undefined) ../gegl/libgegl-$(GEGL_API_VERSION).la \
	$(DEP_LIBS) $(BABL_LIBS) $(PNG_LIBS) $(LIBSPIRO) $(MATH_LIB) \
        $(MRG_LIBS) $(GEXIV2_LIBS)

bin_PROGRAMS = gegl gegl-tester

gegl_SOURCES =			\
	gegl.c			\
	gegl-options.c		\
	gegl-options.h		\
	gegl-path-smooth.c	\
	gegl-path-smooth.h

gegl_tester_SOURCES = \
	gegl-tester.c

if HAVE_MRG
if HAVE_GEXIV2
if HAVE_SDL
gegl_SOURCES += mrg-ui.c
AM_CFLAGS += $(SDL_CFLAGS)
AM_LDFLAGS += $(SDL_LIBS)
endif
endif
endif

if HAVE_SPIRO
gegl_SOURCES += gegl-path-spiro.h gegl-path-spiro.c
endif

