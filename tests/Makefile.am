INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)/gegl \
	-I$(top_srcdir)/tools/testsuite \
	@DEP_CFLAGS@ \
@STRIP_END@

DEPS = \
    $(top_builddir)/tools/testsuite/libtestsuite.la \
	$(top_builddir)/gegl/@gegltargetlib@

LDADDS = @STRIP_BEGIN@ \
    $(top_builddir)/tools/testsuite/libtestsuite.la \
	$(top_builddir)/gegl/@gegltargetlib@ \
@STRIP_END@

TEST_DEPS = \
	$(top_builddir)/gegl/@gegltargetlib@

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
   gegl-mock-properties-filter.c \
   gegl-mock-properties-filter.h \
   gegl-mock-visitor.c \
   gegl-mock-visitor.h

EXTRA_SRC = \
   gegl-check-op.c \
   gegl-check-op.h

testgegl_SOURCES = \
   $(MOCK_OBJECT_SRC) \
   $(EXTRA_SRC) \
   addtest-float.c \
   addtest-uint8.c \
   bfsvisitortest.c \
   buffertest.c \
   channelfloatvaluetest.c \
   channelspacetest.c \
   channeluint8valuetest.c \
   channelvaluetest.c \
   checkoptest.c \
   colormodeltest.c \
   colorspacetest.c \
   colortest-float.c \
   colortest-uint8.c \
   copytest-float.c \
   copytest-uint8.c \
   dfsvisitortest.c \
   dumpvisitortest.c \
   fadetest-float.c \
   fadetest-uint8.c \
   graphapplytest-float.c \
   graphnodetest.c \
   haverecttest.c \
   iaddtest-float.c \
   iaddtest-uint8.c \
   imulttest-float.c \
   imulttest-uint8.c \
   imageiteratortest.c \
   imageoptest.c \
   imagetest.c \
   mockpropertiesfiltertest.c \
   multiplytest-float.c \
   multtest-float.c \
   multtest-uint8.c \
   needrecttest.c \
   nodetest.c \
   normaltest-float.c \
   objecttest.c \
   optest.c \
   pipetest.c \
   pixelrgbfloatvaluetest.c \
   pixelrgbafloatvaluetest.c \
   pixelrgbuint8valuetest.c \
   pointoptest.c \
   printtest.c \
   simpletreetest-float.c \
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

dummy:
	cd $(top_builddir)/gegl \
	&& $(MAKE) $(AM_MAKEFLAGS)
