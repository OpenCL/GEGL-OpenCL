INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)/gegl \
	-I$(top_srcdir)/tools/testsuite \
	@DEP_CFLAGS@ \
@STRIP_END@

TESTDEPS = \
    $(top_builddir)/tools/testsuite/libtestsuite.la

GEGLDEPS = \
	$(TESTDEPS) \
	$(top_builddir)/gegl/@gegltargetlib@

IMAGEDEPS = \
	$(TESTDEPS) \
	$(top_builddir)/gegl/image/libimage.la

TESTLDADDS = \
    $(top_builddir)/tools/testsuite/libtestsuite.la

GEGLLDADDS = @STRIP_BEGIN@ \
	$(TESTLDADDS) \
	$(top_builddir)/gegl/@gegltargetlib@ \
@STRIP_END@

IMAGELDADDS = @STRIP_BEGIN@ \
	$(TESTLDADDS) \
	$(top_builddir)/gegl/image/libimage.la \
@STRIP_END@

MOCK_OBJECT_SRC = \
    gegl-mock-image-filter.c \
    gegl-mock-image.c \
    gegl-mock-filter-0-1.c \
    gegl-mock-filter-0-1.h \
    gegl-mock-filter-1-1.c \
    gegl-mock-filter-1-1.h \
    gegl-mock-filter-1-2.c \
    gegl-mock-filter-1-2.h \
    gegl-mock-filter-2-1.c \
    gegl-mock-filter-2-1.h \
    gegl-mock-filter-2-2.c \
    gegl-mock-filter-2-2.h \
    gegl-mock-node-visitor.c \
    gegl-mock-node-visitor.h \
    gegl-mock-object.c \
    gegl-mock-object.h \
    gegl-mock-property-visitor.c \
    gegl-mock-property-visitor.h

EXTRA_SRC =

testgegl_SOURCES = \
    $(MOCK_OBJECT_SRC) \
    $(EXTRA_SRC) \
    bfspropertyvisitortest.c \
    dfsnodevisitortest.c \
    dfspropertyvisitortest.c \
    graphtest.c \
    mockimagefiltertest.c \
    mockfilter01test.c \
    mockfilter11test.c \
    mockfilter12test.c \
    mockfilter21test.c \
    mockfilter22test.c \
    nodeconnectionstest.c \
    objecttest.c \
    propertyconnectionstest.c \
    testgegl.c \
    updatepropertytest.c

testgegl_LDFLAGS = -g
testgegl_DEPENDENCIES = $(GEGLDEPS)
testgegl_LDADD = $(GEGLLDADDS) -lm

testimage_SOURCES = testimage.c \
                    gegl-image-buffer-double-test.c \
		    gegl-image-component-sample-model-test.c

testimage_LDFLAGS= -g  \
                  @DEP_LIBS@

testimage_DEPENDENCIES = $(IMAGEDEPS)

testimage_LDADD = $(IMAGELDADDS) -lm 

TESTS = testgegl testimage

EXTRA_PROGRAMS = $(TESTS) 



all: check 
