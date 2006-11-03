bin_PROGRAMS = gegl

libbuffer = $(top_builddir)/gegl/buffer/libbuffer.la
libmodule = $(top_builddir)/gegl/module/libmodule.la

if HAVE_GTK
gui_sources = \
	editor.c \
	editor.h \
	editor-optype.c   \
	gegl-projection.c \
	gegl-projection.h \
	gegl-store.c      \
	gegl-store.h      \
	gegl-tree-editor-action.c \
	gegl-tree-editor-action.h \
	gegl-tree-editor.c        \
	gegl-tree-editor.h        \
	gegl-view.c \
	gegl-view.h
endif

gegl_SOURCES = \
	gegl.c	\
	gegl-options.c \
	gegl-options.h \
        $(gui_sources)

INCLUDES = \
	-I$(top_srcdir)	-I$(top_srcdir)/gegl -I$(top_srcdir)/gegl/buffer \
	@DEP_CFLAGS@ @GTK_CFLAGS@ @BABL_CFLAGS@

AM_LDFLAGS =  \
	-no-undefined \
	@DEP_LIBS@ @GTK_LIBS@ @BABL_LIBS@

gegl_LDFLAGS = \
	../gegl/libgegl-@GEGL_API_VERSION@.la
