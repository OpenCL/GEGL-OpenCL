noinst_PROGRAMS =

SUBDIRS = image

libimage = $(top_builddir)/gegl/image/libimage.la

lib_LTLIBRARIES = libgegl-1.0.la

GEGL_SRC = \
  gegl-graph.c \
  gegl-eval-mgr.c \
  gegl-eval-visitor.c \
  gegl-filter.c \
  gegl-visitor.c \
  gegl-node.c \
  gegl-object.c \
  gegl-property.c \
  gegl-visitable.c

GEGL_NONCLASS_SRC = \
  gegl-connection.c \
  gegl-init.c \
  gegl-utils.c

GEGL_PUBLIC_HDRS = \
  gegl.h

libgegl_1_0_la_SOURCES = \
  $(GEGL_SRC) \
  $(GEGL_NONCLASS_SRC)

libgegl_1_0_la_LIBADD = $(libimage)

#BUILT_SOURCES = \
#  $(GEGL_GEN_SRC)

INCLUDES = \
	-I$(top_srcdir)	\
	@DEP_CFLAGS@

AM_LDFLAGS =  \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	-no-undefined \
	@DEP_LIBS@

libgeglincludedir=$(includedir)/gegl-1.0/gegl
libgeglinclude_HEADERS = $(GEGL_PUBLIC_HDRS) 

#%.c: %.gil
#	../codegen/codegen --channel-names r,g,b --channel-data-file ../codegen/channel_data/float $< > $@

#distclean-local: gilclean

#clean: clean-am gilclean

#gilclean:
#	rm -f $(GEGL_GEN_SRC)
