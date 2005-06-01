noinst_PROGRAMS =

SUBDIRS = image

libimage = $(top_builddir)/gegl/image/libimage.la

lib_LTLIBRARIES = libgegl-1.0.la

GEGL_sources = \
	gegl-connection.c	\
	gegl-eval-mgr.c		\
	gegl-eval-visitor.c	\
	gegl-filter.c		\
	gegl-graph.c		\
	gegl-init.c		\
	gegl-node.c		\
	gegl-object.c		\
	gegl-property.c		\
	gegl-utils.c		\
	gegl-visitable.c	\
	gegl-visitor.c

GEGL_broken_sources = \
	gegl-add.c		\
	gegl-comp.c		\
	gegl-fade.c		\
	gegl-fill.c		\
	gegl-i-add.c		\
	gegl-mult.c		\
	gegl-multiply.c		\
	gegl-over.c		\
	gegl-print.c

GEGL_public_headers = \
	gegl.h
	gegl-connection.h	\
	gegl-filter.h		\
	gegl-graph.h		\
	gegl-init.h		\
	gegl-node.h		\
	gegl-object.h		\
	gegl-property.h		\
	gegl-visitable.h	\
	gegl-visitor.h

libgegl_1_0_la_SOURCES = $(GEGL_sources) $(GEGL_public_headers)

libgegl_1_0_la_LIBADD = $(libimage)

INCLUDES = \
	-I$(top_srcdir)	\
	@DEP_CFLAGS@

AM_LDFLAGS =  \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	-no-undefined \
	@DEP_LIBS@


libgeglincludedir = $(includedir)/gegl-1.0/gegl

libgeglinclude_HEADERS = $(GEGL_public_headers) 


#BUILT_SOURCES = \
#  $(GEGL_GEN_SRC)

#%.c: %.gil
#	../codegen/codegen --channel-names r,g,b --channel-data-file ../codegen/channel_data/float $< > $@

#distclean-local: gilclean

#clean: clean-am gilclean

#gilclean:
#	rm -f $(GEGL_GEN_SRC)
