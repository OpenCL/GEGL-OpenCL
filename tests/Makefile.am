INCLUDES = \
	-I$(top_srcdir)/gegl \
	-I$(top_srcdir)/tools/testsuite \
	@GEGL_CFLAGS@

LDFLAGS =  \
	@GEGL_LIBS@

noinst_PROGRAMS = testgegl

DEPS = ../gegl/libgegl.la ../tools/testsuite/libtestsuite.la
LDADDS = ../gegl/libgegl.la ../tools/testsuite/libtestsuite.la

testgegl_SOURCES = \
   addimpltest.c \
   addtest.c \
   buffertest.c \
   colortest.c \
   colormodeltest.c \
   colormodelrgbtest.c \
   colormodelrgbfloattest.c \
   constmultimpltest.c \
   constmulttest.c \
   copytest.c \
   opimpltest.c \
   fillimpltest.c \
   filltest.c \
   imagemgrtest.c \
   imagetest.c \
   membuffertest.c \
   nodetest.c \
   objecttest.c \
   optest.c \
   pointoptest.c \
   pointopimpltest.c \
   printtest.c \
   printimpltest.c \
   sampledimagetest.c \
   sampledimageimpltest.c \
   simpleimagemgrtest.c \
   simpletreetest.c \
   statoptest.c \
   statopimpltest.c \
   testgegl.c \
   testutils.c \
   tileiteratortest.c \
   tilemgrtest.c \
   tiletest.c

testgegl_LDFLAGS = -g
testgegl_DEPENDENCIES = $(DEPS)
testgegl_LDADD = $(LDADDS) -lm

tests: dummy testgegl

all: tests
	testgegl

dummy:
	cd $(top_srcdir)/gegl \
	  && $(MAKE) $(AM_MAKEFLAGS)
