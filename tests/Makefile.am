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
   bfsvisitormultouttest.c \
   bfsvisitortest.c \
   checktest.c \
   colormodeltest.c \
   colorspacetest.c \
   colortest-rgb-float.c \
   colortest-rgb-uint8.c \
   databuffertest.c \
   dataspacetest.c \
   dfsvisitormultouttest.c \
   dfsvisitortest.c \
   dumpvisitortest.c \
   fadetest-rgb-float.c \
   fadetest-rgb-uint8.c \
   floatvaluetest.c \
   graphapplytest-rgb-float.c \
   graphnodetest.c \
   haverecttest.c \
   iaddtest-rgb-float.c \
   iaddtest-rgb-uint8.c \
   imagetest.c \
   imagedatatest.c \
   imagedataiteratortest.c \
   imulttest-rgb-float.c \
   imulttest-rgb-uint8.c \
   multiplytest-rgb-float.c \
   multtest-rgb-float.c \
   multtest-rgb-uint8.c \
   needrecttest.c \
   nodetest.c \
   nodemultouttest.c \
   optest.c \
   objecttest.c \
   pipetest.c \
   pointoptest.c \
   printtest.c \
   rgbfloatvaluetest.c \
   rgbuint8valuetest.c \
   simpletreetest-rgb-float.c \
   storagetest.c \
   testgegl.c \
   testutils.c \
   tiletest.c \
   uint8valuetest.c \
   valueimagedatatest.c \
   valuetest.c

testgegl_LDFLAGS = -g
testgegl_DEPENDENCIES = $(DEPS)
testgegl_LDADD = $(LDADDS) -lm

TESTS = testgegl

EXTRA_PROGRAMS = $(TESTS)
