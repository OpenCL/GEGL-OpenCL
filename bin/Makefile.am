if HAVE_GTK
SUBDIRS = node-editors
endif

bin_PROGRAMS = gegl

libbuffer = $(top_builddir)/gegl/buffer/libbuffer.la
libmodule = $(top_builddir)/gegl/module/libmodule.la

if HAVE_GTK
gui_sources = \
	editor.c				\
	editor.h				\
	editor-optype.c				\
	editor-optype.h				\
	export.c				\
	export.h				\
	gegl-node-editor.c			\
	gegl-node-editor.h			\
	gegl-store.c				\
	gegl-store.h				\
	gegl-tree-editor-action.c		\
	gegl-tree-editor-action.h		\
	gegl-node-editor-plugin.h		\
	gegl-tree-editor.c			\
	gegl-tree-editor.h			\
	gegl-view.c				\
	gegl-view.h
endif

gegl_SOURCES =			\
	gegl.c			\
	gegl-options.c		\
	gegl-options.h		\
        $(gui_sources)

INCLUDES = \
	-I$(top_srcdir)	-I$(top_srcdir)/gegl -I$(top_srcdir)/gegl/buffer \
	-I$(top_srcdir)/gegl/property-types \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_srcdir)/gegl/module \
	@DEP_CFLAGS@ @GTK_CFLAGS@ @BABL_CFLAGS@ @PNG_CFLAGS@

AM_LDFLAGS =  \
	-no-undefined ../gegl/libgegl-@GEGL_API_VERSION@.la \
	@DEP_LIBS@ @GTK_LIBS@ @BABL_LIBS@ @PNG_LIBS@
