noinst_PROGRAMS =

lib_LTLIBRARIES = libgegl.la

GEGL_SRC = \
  gegl-atop.c \
  gegl-add.c \
  gegl-bfs-visitor.c \
  gegl-binary.c \
  gegl-blend.c \
  gegl-channel-value-types.c \
  gegl-channel-param-specs.c \
  gegl-color.c \
  gegl-color-model.c \
  gegl-color-space.c \
  gegl-color-space-rgb.c \
  gegl-color-space-gray.c \
  gegl-comp.c \
  gegl-component-color-model.c \
  gegl-component-storage.c \
  gegl-darken.c \
  gegl-data-buffer.c \
  gegl-data-space.c \
  gegl-data-space-float.c \
  gegl-data-space-u8.c \
  gegl-dfs-visitor.c \
  gegl-difference.c \
  gegl-dump-visitor.c \
  gegl-eval-bfs-visitor.c \
  gegl-eval-dfs-visitor.c \
  gegl-eval-mgr.c \
  gegl-eval-visitor.c \
  gegl-fade.c \
  gegl-filter.c \
  gegl-graph.c \
  gegl-graph-setup-visitor.c \
  gegl-i-add.c \
  gegl-i-mult.c \
  gegl-image.c \
  gegl-image-data.c \
  gegl-image-data-param-specs.c \
  gegl-image-data-iterator.c \
  gegl-inside.c \
  gegl-init.c \
  gegl-lighten.c \
  gegl-max.c \
  gegl-min.c \
  gegl-mult.c \
  gegl-multiply.c \
  gegl-no-input.c \
  gegl-node.c \
  gegl-object.c \
  gegl-op.c \
  gegl-over.c \
  gegl-outside.c \
  gegl-pipe.c \
  gegl-pixel-value-types.c \
  gegl-pixel-param-specs.c \
  gegl-point-op.c \
  gegl-premult.c \
  gegl-print.c \
  gegl-scanline-processor.c \
  gegl-screen.c \
  gegl-storage.c \
  gegl-subtract.c \
  gegl-tile.c \
  gegl-unary.c \
  gegl-unpremult.c \
  gegl-visitor.c \
  gegl-xor.c

GEGL_NONCLASS_SRC = \
  gegl-attributes.c \
  gegl-utils.c

GEGL_PUBLIC_HDRS = \
  gegl.h

libgegl_la_SOURCES = \
  $(GEGL_SRC) \
  $(GEGL_NONCLASS_SRC)

#BUILT_SOURCES = \
#  $(GEGL_GEN_SRC)

INCLUDES = \
	-I$(top_srcdir)	\
	@GEGL_CFLAGS@

LDFLAGS =  \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	-no-undefined						\
	@GEGL_LIBS@

libgeglincludedir=$(includedir)/gegl-1.0/gegl
libgeglinclude_HEADERS = $(GEGL_PUBLIC_HDRS) 

#%.c: %.gil
#	../codegen/codegen --channel-names r,g,b --channel-data-file ../codegen/channel_data/float $< > $@

#distclean-local: gilclean

#clean: clean-am gilclean

#gilclean:
#	rm -f $(GEGL_GEN_SRC)
