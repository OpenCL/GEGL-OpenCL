INCLUDES = \
	-I$(top_srcdir)/gegl \
	-I$(top_srcdir)/tools/testsuite \
	@GEGL_CFLAGS@

LDFLAGS =  \
	@GEGL_LIBS@

noinst_PROGRAMS = testgegl

DEPS = ../gegl/libgegl.la ../tools/testsuite/libtestsuite.la
LDADDS = ../gegl/libgegl.la ../tools/testsuite/libtestsuite.la

MOCK_OBJECT_SRC = \
   gegl-mock-image.c \
   gegl-mock-image.h \
   gegl-mock-node.c \
   gegl-mock-node.h \
   gegl-mock-object.c \
   gegl-mock-object.h \
   gegl-mock-op.c \
   gegl-mock-op.h \
   gegl-mock-point-op.c \
   gegl-mock-point-op.h

testgegl_SOURCES = \
   $(MOCK_OBJECT_SRC) \
   addtest.c \
   buffertest.c \
   colortest.c \
   colormodeltest.c \
   colormodelrgbtest.c \
   colormodelrgbfloattest.c \
   constmulttest.c \
   copytest.c \
   filltest.c \
   imagetest.c \
   membuffertest.c \
   nodetest.c \
   optest.c \
   objecttest.c \
   pointoptest.c \
   printtest.c \
   sampledimagetest.c \
   simpletreetest.c \
   statoptest.c \
   testgegl.c \
   testutils.c \
   tiletest.c \
   tileiteratortest.c \
   tilemgrtest.c \
   valuetest.c

testgegl_LDFLAGS = -g
testgegl_DEPENDENCIES = $(DEPS)
testgegl_LDADD = $(LDADDS) -lm -lefence

tests: dummy testgegl

all: tests
	testgegl

dummy:
	cd $(top_srcdir)/gegl \
	  && $(MAKE) $(AM_MAKEFLAGS)
