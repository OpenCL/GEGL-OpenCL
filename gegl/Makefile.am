noinst_PROGRAMS = testgegl testgeglfile

lib_LTLIBRARIES = libgegl.la

GOB     = \
        gegl-object.gob \
        gegl-drawable.gob \
        gegl-color-model.gob \
        gegl-color-model-rgb.gob \
        gegl-color-model-rgb-float.gob \
        gegl-color-model-rgb-u8.gob \
        gegl-color-model-gray.gob \
        gegl-color-model-gray-float.gob \
        gegl-color-model-gray-u8.gob \
	gegl-image-buffer.gob \
        gegl-op.gob \
        gegl-point-op.gob \
        gegl-print-op.gob \
        gegl-color.gob \
        gegl-graphics-state.gob \
        gegl-fill-op.gob \
	gegl-composite-op.gob \
	gegl-premult-op.gob \
	gegl-unpremult-op.gob \
	gegl-color-convert-op.gob \
	gegl-color-convert-to-rgb-float-op.gob \
	gegl-color-convert-to-gray-float-op.gob \
	gegl-color-convert-to-rgb-u8-op.gob \
	gegl-color-convert-to-gray-u8-op.gob

	

GOB_SRC = \
        gegl-object.c \
        gegl-drawable.c \
        gegl-color-model.c \
        gegl-color-model-rgb.c \
        gegl-color-model-rgb-float.c \
	gegl-color-model-rgb-u8.c \
        gegl-color-model-gray.c \
        gegl-color-model-gray-float.c \
        gegl-color-model-gray-u8.c \
        gegl-image-buffer.c \
        gegl-op.c \
        gegl-point-op.c \
        gegl-print-op.c \
        gegl-color.c \
        gegl-graphics-state.c \
        gegl-fill-op.c \
	gegl-composite-op.c \
        gegl-premult-op.c \
        gegl-unpremult-op.c \
	gegl-color-convert-op.c \
	gegl-color-convert-to-rgb-float-op.c \
	gegl-color-convert-to-gray-float-op.c \
	gegl-color-convert-to-rgb-u8-op.c \
	gegl-color-convert-to-gray-u8-op.c


GOB_PUBLIC_H = \
	gegl-object.h \
        gegl-drawable.h \
        gegl-color-model.h \
        gegl-color-model-rgb.h \
        gegl-color-model-rgb-float.h \
	gegl-color-model-rgb-u8.h \
        gegl-color-model-gray.h \
        gegl-color-model-gray-float.h \
        gegl-color-model-gray-u8.h \
        gegl-image-buffer.h \
        gegl-op.h \
        gegl-point-op.h \
        gegl-print-op.h \
        gegl-color.h \
        gegl-graphics-state.h \
        gegl-fill-op.h \
	gegl-composite-op.h \
        gegl-premult-op.h \
        gegl-unpremult-op.h \
	gegl-color-convert-op.h \
	gegl-color-convert-to-rgb-float-op.h \
	gegl-color-convert-to-gray-float-op.h \
	gegl-color-convert-to-rgb-u8-op.h \
	gegl-color-convert-to-gray-u8-op.h
 



libgegl_la_SOURCES = \
	$(GOB_SRC) \
	gegl-utils.c

libgegl_la_LDFLAGS = -version-info @GEGL_VERSION_INFO@

libgeglincdir = $(includedir)/gegl

libgeglinc_HEADERS = $(GOB_PUBLIC_H) 

INCLUDES = -I$(top_srcdir) -I$(top_builddir) -DGEGL_COMPILATION

DEPS = libgegl.la
LDADDS = libgegl.la

testgegl_SOURCES=testgegl.c
testgegl_LDFLAGS =
testgegl_DEPENDENCIES = $(DEPS)
testgegl_LDADD = $(LDADDS) -lm

testgeglfile_SOURCES=testgeglfile.c
testgeglfile_LDFLAGS =
testgeglfile_DEPENDENCIES = $(DEPS)
testgeglfile_LDADD = $(LDADDS) -ltiff -lm

tests: testgegl testgeglfile

%.c: %.gob
	gob $<

%.h: %.gob	
	gob $<

%-private.h: %.gob	
	gob $<

distclean:
	$(MAKE) clean
	rm -f $(GOB_PUBLIC_H) $(GOB_SRC) *-private.h

gobclean:
	rm -f $(GOB_PUBLIC_H) $(GOB_SRC) *-private.h

gobs:
	for blah in $(GOB); do \
	  gob $$blah; \
	done
