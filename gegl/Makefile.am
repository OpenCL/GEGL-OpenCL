noinst_PROGRAMS = testgegl testgeglfile

lib_LTLIBRARIES = libgegl.la

GIL_FILES = \
  gegl-fill-op.gil \
  gegl-print-op.gil \
  gegl-mult-op.gil \
  gegl-add-op.gil \
  gegl-subtract-op.gil \
  gegl-max-op.gil \
  gegl-min-op.gil \
  gegl-dark-op.gil \
  gegl-screen-op.gil \
  gegl-light-op.gil \
  gegl-premult-op.gil \
  gegl-unpremult-op.gil \
  gegl-composite-premult-op.gil \
  gegl-composite-op.gil \
  gegl-test-op.gil

GIL_GEN_GOB = \
  gegl-fill-op.gob \
  gegl-print-op.gob \
  gegl-mult-op.gob \
  gegl-add-op.gob \
  gegl-subtract-op.gob \
  gegl-max-op.gob \
  gegl-min-op.gob \
  gegl-dark-op.gob \
  gegl-screen-op.gob \
  gegl-light-op.gob \
  gegl-premult-op.gob \
  gegl-unpremult-op.gob \
  gegl-composite-premult-op.gob \
  gegl-composite-op.gob \
  gegl-test-op.gob

NONGEN_GOB = \
  gegl-object.gob \
  gegl-color-model.gob \
  gegl-color-model-gray.gob \
  gegl-color-model-gray-u8.gob \
  gegl-color-model-gray-u16.gob \
  gegl-color-model-gray-u16_4.gob \
  gegl-color-model-gray-float.gob \
  gegl-color-model-rgb.gob \
  gegl-color-model-rgb-u8.gob \
  gegl-color-model-rgb-u16.gob \
  gegl-color-model-rgb-u16_4.gob \
  gegl-color-model-rgb-float.gob \
  gegl-image-buffer.gob \
  gegl-image.gob \
  gegl-op.gob \
  gegl-point-op.gob \
  gegl-single-src-op.gob \
  gegl-dual-src-op.gob \
  gegl-color.gob \
  gegl-graphics-state.gob \
  gegl-image-iterator.gob

GOB_FILES = $(GIL_GEN_GOB) $(NONGEN_GOB)

GOB_GEN_PUBLIC_HDRS = \
  gegl-object.h \
  gegl-color-model.h \
  gegl-color-model-gray.h \
  gegl-color-model-gray-u8.h \
  gegl-color-model-gray-u16.h \
  gegl-color-model-gray-u16_4.h \
  gegl-color-model-gray-float.h \
  gegl-color-model-rgb.h \
  gegl-color-model-rgb-u8.h \
  gegl-color-model-rgb-u16.h \
  gegl-color-model-rgb-u16_4.h \
  gegl-color-model-rgb-float.h \
  gegl-image-buffer.h \
  gegl-image.h \
  gegl-op.h \
  gegl-point-op.h \
  gegl-single-src-op.h \
  gegl-dual-src-op.h \
  gegl-fill-op.h \
  gegl-print-op.h \
  gegl-mult-op.h \
  gegl-add-op.h \
  gegl-subtract-op.h \
  gegl-max-op.h \
  gegl-min-op.h \
  gegl-dark-op.h \
  gegl-screen-op.h \
  gegl-light-op.h \
  gegl-premult-op.h \
  gegl-unpremult-op.h \
  gegl-composite-premult-op.h \
  gegl-composite-op.h \
  gegl-test-op.h \
  gegl-color.h \
  gegl-graphics-state.h \
  gegl-image-iterator.h

GOB_GEN_PRIVATE_HDRS = \
  gegl-object-private.h \
  gegl-color-model-private.h \
  gegl-color-model-gray-private.h \
  gegl-color-model-gray-u8-private.h \
  gegl-color-model-gray-u16-private.h \
  gegl-color-model-gray-u16_4-private.h \
  gegl-color-model-gray-float-private.h \
  gegl-color-model-rgb-private.h \
  gegl-color-model-rgb-u8-private.h \
  gegl-color-model-rgb-u16-private.h \
  gegl-color-model-rgb-u16_4-private.h \
  gegl-color-model-rgb-float-private.h \
  gegl-image-buffer-private.h \
  gegl-image-private.h \
  gegl-op-private.h \
  gegl-point-op-private.h \
  gegl-single-src-op-private.h \
  gegl-dual-src-op-private.h \
  gegl-fill-op-private.h \
  gegl-print-op-private.h \
  gegl-mult-op-private.h \
  gegl-add-op-private.h \
  gegl-subtract-op-private.h \
  gegl-max-op-private.h \
  gegl-min-op-private.h \
  gegl-dark-op-private.h \
  gegl-screen-op-private.h \
  gegl-light-op-private.h \
  gegl-premult-op-private.h \
  gegl-unpremult-op-private.h \
  gegl-composite-premult-op-private.h \
  gegl-composite-op-private.h \
  gegl-test-op-private.h \
  gegl-color-private.h \
  gegl-graphics-state-private.h \
  gegl-image-iterator-private.h

GOB_GEN_SRC = \
  gegl-object.c \
  gegl-color-model.c \
  gegl-color-model-gray.c \
  gegl-color-model-gray-u8.c \
  gegl-color-model-gray-u16.c \
  gegl-color-model-gray-u16_4.c \
  gegl-color-model-gray-float.c \
  gegl-color-model-rgb.c \
  gegl-color-model-rgb-u8.c \
  gegl-color-model-rgb-u16.c \
  gegl-color-model-rgb-u16_4.c \
  gegl-color-model-rgb-float.c \
  gegl-image-buffer.c \
  gegl-image.c \
  gegl-op.c \
  gegl-point-op.c \
  gegl-single-src-op.c \
  gegl-dual-src-op.c \
  gegl-fill-op.c \
  gegl-print-op.c \
  gegl-mult-op.c \
  gegl-add-op.c \
  gegl-subtract-op.c \
  gegl-max-op.c \
  gegl-min-op.c \
  gegl-dark-op.c \
  gegl-screen-op.c \
  gegl-light-op.c \
  gegl-premult-op.c \
  gegl-unpremult-op.c \
  gegl-composite-premult-op.c \
  gegl-composite-op.c \
  gegl-test-op.c \
  gegl-color.c \
  gegl-graphics-state.c \
  gegl-image-iterator.c

NONGOB_HDRS = \
  gegl-types.h \
  gegl-utils.h \
  trace.h

NONGOB_SRC = \
  gegl-utils.c \
  trace.c

libgegl_la_SOURCES = \
  $(GIL_FILES) \
  $(GIL_GEN_GOB) \
  $(GOB_GEN_SRC) \
  $(GOB_GEN_PUBLIC_HDRS) \
  $(GOB_GEN_PRIVATE_HDRS) \
  $(NONGOB_HDRS) \
  $(NONGOB_SRC)

BUILT_SOURCES = \
  $(GIL_GEN_GOB) \
  $(GOB_GEN_SRC) \
  $(GOB_GEN_PUBLIC_HDRS) \
  $(GOB_GEN_PRIVATE_HDRS)

libgegl_la_LDFLAGS = -version-info @GEGL_VERSION_INFO@

libgeglincdir = $(includedir)/gegl
libgeglinc_HEADERS = $(NONGOB_HDRS) $(GOB_GEN_PUBLIC_HDRS) 

AM_CPPFLAGS = \
  -DGEGL_COMPILATION

INCLUDES = \
  -I$(top_srcdir) \
  -I$(top_builddir)

DEPS = libgegl.la
LDADDS = libgegl.la

testgegl_SOURCES = testgegl.c
testgegl_LDFLAGS = -g
testgegl_DEPENDENCIES = $(DEPS)
testgegl_LDADD = $(LDADDS) -lm

testgeglfile_SOURCES = testgeglfile.c
testgeglfile_LDFLAGS =
testgeglfile_DEPENDENCIES = $(DEPS)
testgeglfile_LDADD = $(LDADDS) -ltiff -lm

tests:

%.c %.h: %.gob
	$(GOB) $<

%.gob: %.gil
	../codegen/codegen --channel-names r,g,b --channel-data-file ../codegen/channel_data/float $< > $@ 

distclean-local: gilclean

hello:
	echo "hello"

gilclean:
	rm -f $(GIL_GEN_GOB)
	rm -f $(GOB_GEN_SRC) $(GOB_GEN_PUBLIC_HDRS) $(GOB_GEN_PRIVATE_HDRS)

gobclean:
	rm -f $(GOB_GEN_SRC) $(GOB_GEN_PUBLIC_HDRS) $(GOB_GEN_PRIVATE_HDRS)
