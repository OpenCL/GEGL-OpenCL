noinst_PROGRAMS = testgegl

lib_LTLIBRARIES = libgegl.la

GOB     = \
        gegl-object.gob \
        gegl-color-model.gob \
        gegl-color-model-rgb.gob \
        gegl-color-model-rgb-float.gob \
        gegl-image-buffer.gob \
        gegl-op.gob \
        gegl-point-op.gob \
        gegl-print-op.gob \
        gegl-color.gob \
        gegl-graphics-state.gob \
        gegl-fill-op.gob
	

GOB_SRC = \
        gegl-object.c \
        gegl-color-model.c \
        gegl-color-model-rgb.c \
        gegl-color-model-rgb-float.c \
        gegl-image-buffer.c \
        gegl-op.c \
        gegl-point-op.c \
        gegl-print-op.c \
        gegl-color.c \
        gegl-graphics-state.c \
        gegl-fill-op.c

GOB_PUBLIC_H = \
	gegl-object.h \
        gegl-color-model.h \
        gegl-color-model-rgb.h \
        gegl-color-model-rgb-float.h \
        gegl-image-buffer.h \
        gegl-op.h \
        gegl-point-op.h \
        gegl-print-op.h \
        gegl-color.h \
        gegl-graphics-state.h \
        gegl-fill-op.h



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

tests: testgegl

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
