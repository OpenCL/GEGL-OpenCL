noinst_PROGRAMS =

lib_LTLIBRARIES = libgegl.la

GIL_FILES = \
  gegl-add.gil \
  gegl-comp-premult.gil \
  gegl-comp.gil \
  gegl-const-mult.gil \
  gegl-dark.gil \
  gegl-diff.gil \
  gegl-light.gil \
  gegl-max.gil \
  gegl-min.gil \
  gegl-mult.gil \
  gegl-premult.gil \
  gegl-screen.gil \
  gegl-subtract.gil \
  gegl-unpremult.gil

GEGL_GEN_SRC = \
  gegl-add.c \
  gegl-comp-premult.c \
  gegl-comp.c \
  gegl-const-mult.c \
  gegl-dark.c \
  gegl-diff.c \
  gegl-light.c \
  gegl-max.c \
  gegl-min.c \
  gegl-mult.c \
  gegl-premult.c \
  gegl-screen.c \
  gegl-subtract.c \
  gegl-unpremult.c

GEGL_SRC = \
  gegl-attributes.c \
  gegl-bfs-visitor.c \
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
  gegl-copy.c \
  gegl-dfs-visitor.c \
  gegl-dump-visitor.c \
  gegl-fill.c \
  gegl-filter.c \
  gegl-graph.c \
  gegl-graph-bfs-visitor.c \
  gegl-graph-dfs-visitor.c \
  gegl-graph-evaluate-visitor.c \
  gegl-graph-init-visitor.c \
  gegl-graph-setup-visitor.c \
  gegl-image.c \
  gegl-mem-buffer.c \
  gegl-node.c \
  gegl-object.c \
  gegl-op.c \
  gegl-print.c \
  gegl-param-specs.c \
  gegl-point-op.c \
  gegl-sampled-image.c \
  gegl-scanline-processor.c \
  gegl-stat-op.c \
  gegl-tile.c \
  gegl-tile-iterator.c \
  gegl-tile-mgr.c \
  gegl-value-types.c \
  gegl-visitor.c


GEGL_PUBLIC_HDRS = \
  gegl-add.h \
  gegl-attributes.h \
  gegl-bfs-visitor.h \
  gegl-buffer.h \
  gegl-color.h \
  gegl-color-model.h \
  gegl-color-model-gray.h \
  gegl-color-model-gray-float.h \
  gegl-color-model-gray-u8.h \
  gegl-color-model-gray-u16.h \
  gegl-color-model-rgb.h \
  gegl-color-model-rgb-float.h \
  gegl-color-model-rgb-u8.h \
  gegl-color-model-rgb-u16.h \
  gegl-comp-premult.h \
  gegl-comp.h \
  gegl-const-mult.h \
  gegl-copy.h \
  gegl-dark.h \
  gegl-dfs-visitor.h \
  gegl-diff.h \
  gegl-dump-visitor.h \
  gegl-fill.h \
  gegl-filter.h \
  gegl-graph.h \
  gegl-graph-bfs-visitor.h \
  gegl-graph-dfs-visitor.h \
  gegl-graph-evaluate-visitor.h \
  gegl-graph-init-visitor.h \
  gegl-graph-setup-visitor.h \
  gegl-image.h \
  gegl-light.h \
  gegl-max.h \
  gegl-mem-buffer.h \
  gegl-min.h \
  gegl-mult.h \
  gegl-node.h \
  gegl-object.h \
  gegl-op.h \
  gegl-param-specs.h \
  gegl-point-op.h \
  gegl-premult.h \
  gegl-print.h \
  gegl-sampled-image.h \
  gegl-scanline-processor.h \
  gegl-screen.h \
  gegl-stat-op.h \
  gegl-subtract.h \
  gegl-tile.h \
  gegl-tile-iterator.h \
  gegl-tile-mgr.h \
  gegl-unpremult.h \
  gegl-value-types.h \
  gegl-visitor.h

GEGL_NONCLASS_HDRS = \
  gegl-types.h \
  gegl-utils.h

GEGL_NONCLASS_SRC = \
  gegl-utils.c

libgegl_la_SOURCES = \
  $(GEGL_GEN_SRC) \
  $(GEGL_SRC) \
  $(GEGL_NONCLASS_SRC)


BUILT_SOURCES = \
  $(GEGL_GEN_SRC)

INCLUDES = \
	-I$(top_srcdir)		\
	@GEGL_CFLAGS@

LDFLAGS =  \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	-no-undefined						\
	@GEGL_LIBS@

libgeglincludedir=$(includedir)/gegl-1.0/gegl
libgeglinclude_HEADERS = $(GEGL_NONCLASS_HDRS) $(GEGL_PUBLIC_HDRS) 

%.c: %.gil
	../codegen/codegen --channel-names r,g,b --channel-data-file ../codegen/channel_data/float $< > $@

distclean-local: gilclean

clean: clean-am gilclean

gilclean:
	rm -f $(GEGL_GEN_SRC)
