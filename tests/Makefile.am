INCLUDES = \
	-I$(top_srcdir)/gegl \
	-I$(top_srcdir)/tools/testsuite \
	@GEGL_CFLAGS@

LDFLAGS =  \
	@GEGL_LIBS@

DEPS = ../gegl/libgegl.la ../tools/testsuite/libtestsuite.la
LDADDS = ../gegl/libgegl.la ../tools/testsuite/libtestsuite.la

MOCK_OBJECT_SRC = \
   gegl-mock-bfs-visitor.c \
   gegl-mock-bfs-visitor.h \
   gegl-mock-dfs-visitor.c \
   gegl-mock-dfs-visitor.h \
   gegl-mock-filter.c \
   gegl-mock-filter.h \
   gegl-mock-image.c \
   gegl-mock-image.h \
   gegl-mock-node.c \
   gegl-mock-node.h \
   gegl-mock-object.c \
   gegl-mock-object.h \
   gegl-mock-op.c \
   gegl-mock-op.h \
   gegl-mock-point-op.c \
   gegl-mock-point-op.h \
   gegl-mock-visitor.c \
   gegl-mock-visitor.h

EXTRA_SRC = \
   gegl-check.c \
   gegl-check.h

testgegl_SOURCES = \
   $(MOCK_OBJECT_SRC) \
   $(EXTRA_SRC) \
   addtest-rgb-float.c \
   addtest-rgb-uint8.c \
   bfsvisitortest.c \
   channelfloatvaluetest.c \
   channeluint8valuetest.c \
   checktest.c \
   colormodeltest.c \
   colorspacetest.c \
   colortest-rgb-float.c \
   colortest-rgb-uint8.c \
   databuffertest.c \
   dataspacetest.c \
   dfsvisitortest.c \
   dumpvisitortest.c \
   fadetest-rgb-float.c \
   fadetest-rgb-uint8.c \
   graphapplytest-rgb-float.c \
   graphnodetest.c \
   haverecttest.c \
   iaddtest-rgb-float.c \
   iaddtest-rgb-uint8.c \
   imulttest-rgb-float.c \
   imulttest-rgb-uint8.c \
   imagetest.c \
   imagedataiteratortest.c \
   imagedatatest.c \
   multiplytest-rgb-float.c \
   multtest-rgb-float.c \
   multtest-rgb-uint8.c \
   needrecttest.c \
   nodetest.c \
   objecttest.c \
   optest.c \
   pipetest.c \
   pixelrgbfloatvaluetest.c \
   pixelrgbafloatvaluetest.c \
   pixelrgbuint8valuetest.c \
   pointoptest.c \
   printtest.c \
   simpletreetest-rgb-float.c \
   storagetest.c \
   testgegl.c \
   testutils.c \
   tiletest.c \
   valueimagedatatest.c \
   valuetest.c

testgegl_LDFLAGS = -g
testgegl_DEPENDENCIES = $(DEPS)
testgegl_LDADD = $(LDADDS) -lm

TESTS = testgegl

EXTRA_PROGRAMS = $(TESTS)
