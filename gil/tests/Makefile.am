INCLUDES = \
	-I$(top_srcdir)/gil/gil \
	-I$(top_srcdir)/tools/testsuite \
	@GEGL_CFLAGS@

LDFLAGS =  \
	@GEGL_LIBS@

noinst_PROGRAMS = testgil

DEPS = ../gil/libgil.a ../../tools/testsuite/libtestsuite.la
LDADDS = ../gil/libgil.a ../../tools/testsuite/libtestsuite.la

MOCK_OBJECT_SRC = \
   gil-mock-bfs-visitor.c \
   gil-mock-bfs-visitor.h \
   gil-mock-dfs-visitor.c \
   gil-mock-dfs-visitor.h \
   gil-mock-node.c \
   gil-mock-node.h \
   gil-mock-visitor.c \
   gil-mock-visitor.h

testgil_SOURCES = \
   $(MOCK_OBJECT_SRC) \
   bfsvisitortest.c \
   binaryoptest.c \
   blocktest.c \
   constanttest.c \
   dfsvisitortest.c \
   exprstatementtest.c \
   nodetest.c \
   testgil.c \
   unaryoptest.c \
   variabletest.c

testgil_LDFLAGS = -g
testgil_DEPENDENCIES = $(DEPS)
testgil_LDADD = $(LDADDS) -lm

tests: dummy testgil

all: tests
	testgil

dummy:
	cd $(top_srcdir)/gil/gil \
	  && $(MAKE) $(AM_MAKEFLAGS)
