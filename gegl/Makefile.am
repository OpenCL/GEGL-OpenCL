if OS_WIN32
no_undefined = -no-undefined
endif

SUBDIRS = buffer module

libbuffer = $(top_builddir)/gegl/buffer/libbuffer.la
libmodule = $(top_builddir)/gegl/module/libmodule.la

lib_LTLIBRARIES = libgegl-@GEGL_API_VERSION@.la

GEGL_sources = \
	gegl-color.c			\
	gegl-connection.c		\
	gegl-cr-visitor.c		\
	gegl-curve.c			\
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
	gegl-dot.c			\
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
	gegl-paramspecs.c		\
	gegl-prepare-visitor.c		\
	gegl-processor.c		\
	gegl-utils.c			\
	gegl-visitable.c		\
	gegl-visitor.c			\
	gegl-xml.c			\
	\
	gegl-color.h			\
	gegl-connection.h		\
	gegl-curve.h			\
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
	gegl-processor.h		\
	gegl-types.h			\
	gegl-visitable.h		\
	gegl-visitor.h			\
	gegl-dot.h			\
	gegl-xml.h			\
	gegl-plugin.h			\
	gegl-chant.h			\
	gegl-module.h

GEGL_public_headers = gegl-paramspecs.h	\
	gegl-utils.h			

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

libgegl_@GEGL_API_VERSION@_la_SOURCES = $(GEGL_sources) $(GEGL_public_headers)

libgegl_@GEGL_API_VERSION@_la_LIBADD = $(libbuffer) $(libmodule)

libgegl_@GEGL_API_VERSION@_la_LDFLAGS = \
	@DEP_LIBS@ @BABL_LIBS@	\
	$(no_undefined) -version-info $(GEGL_LIBRARY_VERSION)

INCLUDES = \
	-I$(top_srcdir)	\
	@DEP_CFLAGS@ @BABL_CFLAGS@

AM_CPPFLAGS = -DPREFIX=\""$(prefix)"\"

libgegl_@GEGL_API_VERSION@includedir = $(includedir)/gegl-$(GEGL_API_VERSION)
libgegl_@GEGL_API_VERSION@subincludedir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl

libgegl_@GEGL_API_VERSION@include_HEADERS = gegl.h
libgegl_@GEGL_API_VERSION@subinclude_HEADERS = $(GEGL_public_headers) 


#BUILT_SOURCES = \
#  $(GEGL_GEN_SRC)

#%.c: %.gil
#	../codegen/codegen --channel-names r,g,b --channel-data-file ../codegen/channel_data/float $< > $@

#distclean-local: gilclean

#clean: clean-am gilclean

#gilclean:
#	rm -f $(GEGL_GEN_SRC)
