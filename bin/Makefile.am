bin_PROGRAMS = gegl

libbuffer = $(top_builddir)/gegl/buffer/libbuffer.la
libmodule = $(top_builddir)/gegl/module/libmodule.la

gegl_SOURCES = \
	gegl.c	\
	gegl-options.c \
	gegl-options.h 

INCLUDES = \
	-I$(top_srcdir)	-I$(top_srcdir)/gegl @DEP_CFLAGS@ @BABL_CFLAGS@

AM_LDFLAGS =  \
	-no-undefined \
	@DEP_LIBS@ @BABL_LIBS@

gegl_LDFLAGS = \
	../gegl/libgegl-@GEGL_API_VERSION@.la
