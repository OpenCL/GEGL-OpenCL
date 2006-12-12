noinst_PROGRAMS =

SUBDIRS = buffer module

libbuffer = $(top_builddir)/gegl/buffer/libbuffer.la
libmodule = $(top_builddir)/gegl/module/libmodule.la

lib_LTLIBRARIES = libgegl-@GEGL_API_VERSION@.la

EXTRA_DIST =	\
	gegl-cr-visitor.h		\
	gegl-debug-rect-visitor.h	\
	gegl-eval-mgr.h			\
	gegl-eval-visitor.h		\
	gegl-finish-visitor.h		\
	gegl-have-visitor.h		\
	gegl-instrument.h		\
	gegl-need-visitor.h		\
	gegl-prepare-visitor.h

GEGL_sources = \
	gegl-color.c			\
	gegl-connection.c		\
	gegl-cr-visitor.c		\
	gegl-debug-rect-visitor.c	\
	gegl-eval-mgr.c			\
	gegl-eval-visitor.c		\
	gegl-extension-handler.c	\
	gegl-finish-visitor.c		\
	gegl-graph.c			\
	gegl-have-visitor.c		\
	gegl-init.c			\
	gegl-instrument.c		\
	gegl-need-visitor.c		\
	gegl-node.c			\
	gegl-node-dynamic.c		\
	gegl-object.c			\
	gegl-operation.c        	\
	gegl-operation-composer.c     	\
	gegl-operation-filter.c       	\
	gegl-operation-meta.c       	\
	gegl-operation-point-composer.c \
	gegl-operation-point-filter.c   \
	gegl-operation-sink.c       	\
	gegl-operation-source.c       	\
	gegl-pad.c			\
	gegl-prepare-visitor.c		\
	gegl-utils.c			\
	gegl-visitable.c		\
	gegl-visitor.c			\
	gegl-xml.c

GEGL_public_headers = \
	gegl-color.h			\
	gegl-connection.h		\
	gegl-extension-handler.h	\
	gegl-graph.h			\
	gegl-init.h			\
	gegl-node.h			\
	gegl-node-dynamic.h		\
	gegl-object.h			\
	gegl-operation.h        	\
	gegl-operation-composer.h     	\
	gegl-operation-filter.h       	\
	gegl-operation-meta.h       	\
	gegl-operation-point-composer.h \
	gegl-operation-point-filter.h   \
	gegl-operation-sink.h       	\
	gegl-operation-source.h       	\
	gegl-pad.h			\
	gegl-types.h			\
	gegl-utils.h			\
	gegl-visitable.h		\
	gegl-visitor.h			\
	gegl-xml.h

libgegl_@GEGL_API_VERSION@_la_SOURCES = $(GEGL_sources) $(GEGL_public_headers)

libgegl_@GEGL_API_VERSION@_la_LIBADD = $(libbuffer) $(libmodule)

libgegl_@GEGL_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)

INCLUDES = \
	-I$(top_srcdir)	\
	@DEP_CFLAGS@ @BABL_CFLAGS@

AM_LDFLAGS =  \
	-no-undefined \
	@DEP_LIBS@ @BABL_LIBS@

AM_CPPFLAGS = -DPREFIX=\""$(prefix)"\"

libgeglincludedir = $(includedir)/gegl-$(GEGL_API_VERSION)
libgeglsubincludedir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl

libgeglinclude_HEADERS = gegl.h gegl-module.h gegl-chant.h gegl-plugin.h
libgeglsubinclude_HEADERS = $(GEGL_public_headers) 


#BUILT_SOURCES = \
#  $(GEGL_GEN_SRC)

#%.c: %.gil
#	../codegen/codegen --channel-names r,g,b --channel-data-file ../codegen/channel_data/float $< > $@

#distclean-local: gilclean

#clean: clean-am gilclean

#gilclean:
#	rm -f $(GEGL_GEN_SRC)
