INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)/gegl \
	-I$(top_srcdir)/tools/testsuite \
	@DEP_CFLAGS@ \
@STRIP_END@

LDFLAGS =

DEPS = \
    $(top_builddir)/tools/testsuite/libtestsuite.la \
	$(top_builddir)/gegl/@gegltargetlib@

LDADDS = @STRIP_BEGIN@ \
    $(top_builddir)/tools/testsuite/libtestsuite.la \
	$(top_builddir)/gegl/@gegltargetlib@ \
@STRIP_END@

MOCK_OBJECT_SRC = \
   gegl-mock-bfs-visitor.c \
   gegl-mock-bfs-visitor.h \
   gegl-mock-dfs-visitor.c \
   gegl-mock-dfs-visitor.h \
   gegl-mock-filter.c \
   gegl-mock-filter.h \
   gegl-mock-image-op.c \
   gegl-mock-image-op.h \
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
   buffertest.c \
   channelfloatvaluetest.c \
   channelspacetest.c \
   channeluint8valuetest.c \
   channelvaluetest.c \
   checktest.c \
   colormodeltest.c \
   colorspacetest.c \
   colortest-rgb-float.c \
   colortest-rgb-uint8.c \
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
   imageiteratortest.c \
   imageoptest.c \
   imagetest.c \
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
testgegl_DEPENDENCIES = $(TEST_DEPS)
testgegl_LDADD = $(LDADDS) -lm

TESTS = testgegl

EXTRA_PROGRAMS = $(TESTS)
  
all: check

