noinst_PROGRAMS =

lib_LTLIBRARIES = libgegl-1.0.la

GEGL_SRC = \
  gegl-atop.c \
  gegl-add.c \
  gegl-bfs-visitor.c \
  gegl-binary.c \
  gegl-blend.c \
  gegl-buffer.c \
  gegl-channel-data.c \
  gegl-channel-space.c \
  gegl-channel-space-float.c \
  gegl-channel-space-uint8.c \
  gegl-color.c \
  gegl-color-data.c \
  gegl-color-model.c \
  gegl-color-space.c \
  gegl-color-space-rgb.c \
  gegl-color-space-gray.c \
  gegl-comp.c \
  gegl-component-color-model.c \
  gegl-component-storage.c \
  gegl-copy.c \
  gegl-darken.c \
  gegl-data.c \
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
  gegl-image-op.c \
  gegl-image-data.c \
  gegl-image-iterator.c \
  gegl-inside.c \
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
  gegl-pixel-data.c \
  gegl-point-op.c \
  gegl-premult.c \
  gegl-print.c \
  gegl-scalar-data.c \
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
  gegl-channel-param-specs.c \
  gegl-channel-value-types.c \
  gegl-image-param-specs.c \
  gegl-init.c \
  gegl-input-value-types.c \
  gegl-input-param-specs.c \
  gegl-pixel-param-specs.c \
  gegl-pixel-value-types.c \
  gegl-utils.c

GEGL_PUBLIC_HDRS = \
  gegl.h

libgegl_1_0_la_SOURCES = \
  $(GEGL_SRC) \
  $(GEGL_NONCLASS_SRC)

#BUILT_SOURCES = \
#  $(GEGL_GEN_SRC)

INCLUDES = \
	-I$(top_srcdir)	\
	@DEP_CFLAGS@

libgegl_1_0_la_LDFLAGS =  \
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
