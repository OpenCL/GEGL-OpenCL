noinst_PROGRAMS =

lib_LTLIBRARIES = libgegl.la

GIL_FILES = \
  gegl-add-impl.gil \
  gegl-comp-impl.gil \
  gegl-comp-premult-impl.gil \
  gegl-const-mult-impl.gil \
  gegl-const-mult-impl.gil \
  gegl-dark-impl.gil \
  gegl-diff-impl.gil \
  gegl-light-impl.gil \
  gegl-max-impl.gil \
  gegl-min-impl.gil \
  gegl-mult-impl.gil \
  gegl-premult-impl.gil \
  gegl-screen-impl.gil \
  gegl-subtract-impl.gil \
  gegl-unpremult-impl.gil

GEGL_GEN_SRC = \
  gegl-add-impl.c \
  gegl-comp-impl.c \
  gegl-comp-premult-impl.c \
  gegl-const-mult-impl.c \
  gegl-dark-impl.c \
  gegl-diff-impl.c \
  gegl-light-impl.c \
  gegl-max-impl.c \
  gegl-min-impl.c \
  gegl-mult-impl.c \
  gegl-premult-impl.c \
  gegl-screen-impl.c \
  gegl-subtract-impl.c \
  gegl-unpremult-impl.c

GEGL_SRC = \
  gegl-add.c \
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
  gegl-comp.c \
  gegl-comp-premult.c \
  gegl-const-mult.c \
  gegl-copy.c \
  gegl-copy-impl.c \
  gegl-dark.c \
  gegl-diff.c \
  gegl-fill-impl.c \
  gegl-fill.c \
  gegl-image.c \
  gegl-image-impl.c \
  gegl-image-mgr.c \
  gegl-light.c \
  gegl-max.c \
  gegl-mem-buffer.c \
  gegl-min.c \
  gegl-mult.c \
  gegl-node.c \
  gegl-object.c \
  gegl-op.c \
  gegl-op-impl.c \
  gegl-point-op-impl.c \
  gegl-point-op.c \
  gegl-premult.c \
  gegl-print-impl.c \
  gegl-print.c \
  gegl-sampled-image-impl.c \
  gegl-sampled-image.c \
  gegl-scanline-processor.c \
  gegl-screen.c \
  gegl-simple-image-mgr.c \
  gegl-stat-op-impl.c \
  gegl-stat-op.c \
  gegl-subtract.c \
  gegl-tile.c \
  gegl-tile-iterator.c \
  gegl-tile-mgr.c \
  gegl-unpremult.c

GEGL_PUBLIC_HDRS = \
  gegl-add-impl.h \
  gegl-add.h \
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
  gegl-comp-impl.h \
  gegl-comp.h \
  gegl-comp-premult-impl.h \
  gegl-comp-premult.h \
  gegl-const-mult-impl.h \
  gegl-const-mult.h \
  gegl-copy.h \
  gegl-copy-impl.h \
  gegl-dark-impl.h \
  gegl-dark.h \
  gegl-diff-impl.h \
  gegl-diff.h \
  gegl-fill-impl.h \
  gegl-fill.h \
  gegl-image.h \
  gegl-image-impl.h \
  gegl-image-mgr.h \
  gegl-light-impl.h \
  gegl-light.h \
  gegl-max-impl.h \
  gegl-max.h \
  gegl-min-impl.h \
  gegl-min.h \
  gegl-mem-buffer.h \
  gegl-mult-impl.h \
  gegl-mult.h \
  gegl-node.h \
  gegl-object.h \
  gegl-op.h \
  gegl-op-impl.h \
  gegl-point-op-impl.h \
  gegl-point-op.h \
  gegl-premult-impl.h \
  gegl-premult.h \
  gegl-print-impl.h \
  gegl-print.h \
  gegl-sampled-image-impl.h \
  gegl-sampled-image.h \
  gegl-scanline-processor.h \
  gegl-screen-impl.h \
  gegl-screen.h \
  gegl-simple-image-mgr.h \
  gegl-stat-op-impl.h \
  gegl-stat-op.h \
  gegl-subtract-impl.h \
  gegl-subtract.h \
  gegl-tile.h \
  gegl-tile-iterator.h \
  gegl-tile-mgr.h \
  gegl-unpremult-impl.h \
  gegl-unpremult.h

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
