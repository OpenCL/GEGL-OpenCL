noinst_PROGRAMS =

lib_LTLIBRARIES = libgegl.la

GEGL_SRC = \
  gegl-add.c \
  gegl-atop.c \
  gegl-bfs-visitor.c \
  gegl-binary.c \
  gegl-blend.c \
  gegl-buffer.c \
  gegl-color.c \
  gegl-color-model.c \
  gegl-color-model-gray.c \
  gegl-color-model-gray-float.c \
  gegl-color-model-gray-u8.c \
  gegl-color-model-gray-u16.c \
  gegl-color-model-rgb.c \
  gegl-color-model-rgb-float.c \
  gegl-color-model-rgb-u8.c \
  gegl-color-model-rgb-u16.c \
  gegl-const-mult.c \
  gegl-comp.c \
  gegl-copy.c \
  gegl-darken.c \
  gegl-dfs-visitor.c \
  gegl-difference.c \
  gegl-dump-visitor.c \
  gegl-eval-bfs-visitor.c \
  gegl-eval-dfs-visitor.c \
  gegl-eval-mgr.c \
  gegl-eval-visitor.c \
  gegl-fade.c \
  gegl-fill.c \
  gegl-filter.c \
  gegl-graph.c \
  gegl-graph-setup-visitor.c \
  gegl-image.c \
  gegl-inside.c \
  gegl-lighten.c \
  gegl-max.c \
  gegl-mem-buffer.c \
  gegl-min.c \
  gegl-multiply.c \
  gegl-node.c \
  gegl-no-input.c \
  gegl-object.c \
  gegl-over.c \
  gegl-op.c \
  gegl-outside.c \
  gegl-pipe.c \
  gegl-premult.c \
  gegl-print.c \
  gegl-point-op.c \
  gegl-sampled-image.c \
  gegl-scanline-processor.c \
  gegl-screen.c \
  gegl-subtract.c \
  gegl-tiled-image.c \
  gegl-tile.c \
  gegl-tile-iterator.c \
  gegl-tile-mgr.c \
  gegl-unpremult.c \
  gegl-unary.c \
  gegl-visitor.c \
  gegl-xor.c

GEGL_NONCLASS_SRC = \
  gegl-attributes.c \
  gegl-param-specs.c \
  gegl-value-types.c \
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
