bin_PROGRAMS = gegl

libbuffer = $(top_builddir)/gegl/buffer/libbuffer.la
libmodule = $(top_builddir)/gegl/module/libmodule.la


gegl_SOURCES =			\
	gegl.c			\
	gegl-options.c		\
	gegl-options.h		\
	gegl-path-smooth.c	\
	gegl-path-smooth.h	\
        $(gui_sources)

if HAVE_SPIRO
gegl_SOURCES += gegl-path-spiro.h gegl-path-spiro.c
endif

INCLUDES = \
	-I$(top_srcdir)	-I$(top_srcdir)/gegl -I$(top_srcdir)/gegl/buffer \
	-I$(top_srcdir)/gegl/property-types \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_srcdir)/gegl/module \
	@DEP_CFLAGS@ @BABL_CFLAGS@ @PNG_CFLAGS@

AM_LDFLAGS =  \
	-no-undefined ../gegl/libgegl-@GEGL_API_VERSION@.la \
	@DEP_LIBS@ @BABL_LIBS@ @PNG_LIBS@ @LIBSPIRO@
