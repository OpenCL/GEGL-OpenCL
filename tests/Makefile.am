
INCLUDES = \
	-I$(top_srcdir)/gegl		\
	-I$(top_srcdir)/tools/testsuite \
	@DEP_CFLAGS@

TESTDEPS = \
	$(top_builddir)/tools/testsuite/libtestsuite.la

GEGLDEPS = \
	$(TESTDEPS)			\
	$(top_builddir)/gegl/@gegltargetlib@

TESTLDADDS = \
	$(top_builddir)/tools/testsuite/libtestsuite.la

GEGLLDADDS = \
	$(TESTLDADDS)			\
	$(top_builddir)/gegl/@gegltargetlib@

MOCK_OBJECT_SRC = \
	gegl-mock-image-operation.c	\
	gegl-mock-image.c		\
	gegl-mock-operation-0-1.c	\
	gegl-mock-operation-0-1.h	\
	gegl-mock-operation-1-1.c	\
	gegl-mock-operation-1-1.h	\
	gegl-mock-operation-1-2.c	\
	gegl-mock-operation-1-2.h	\
	gegl-mock-operation-2-1.c	\
	gegl-mock-operation-2-1.h	\
	gegl-mock-operation-2-2.c	\
	gegl-mock-operation-2-2.h	\
	gegl-mock-node-visitor.c	\
	gegl-mock-node-visitor.h	\
	gegl-mock-object.c		\
	gegl-mock-object.h		\
	gegl-mock-pad-visitor.c		\
	gegl-mock-pad-visitor.h		\
	gegl-image-mock-cache-entry.c

testgegl_SOURCES = \
	$(MOCK_OBJECT_SRC)				\
	$(EXTRA_SRC)					\
	bfspadvisitortest.c				\
	dfsnodevisitortest.c				\
	dfspadvisitortest.c				\
	graphtest.c					\
	mockimageoperationtest.c			\
	mockoperation01test.c				\
	mockoperation11test.c				\
	mockoperation12test.c				\
	mockoperation21test.c				\
	mockoperation22test.c				\
	nodeconnectionstest.c				\
	objecttest.c					\
	padconnectionstest.c				\
	testgegl.c					\
	updatepadtest.c					\
	gegl-tests-init-types.c	

testgegl_LDFLAGS = -g
testgegl_DEPENDENCIES = $(GEGLDEPS)
testgegl_LDADD = $(GEGLLDADDS) -lm

TESTS = testgegl$(EXEEXT)

EXTRA_PROGRAMS = testgegl

gegl-tests-init-types.c: ../tools/make-init-types.sh *.h
	../tools/make-init-types.sh gegl_tests> gegl-tests-init-types.c
