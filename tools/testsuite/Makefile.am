
lib_LTLIBRARIES = libtestsuite.la

libtestsuite_la_SOURCES = \
	ctest.c \
	csuite.c

DEFS =

INCLUDES =

libtestsuiteincludedir=$(includedir)/testsuite

libtestsuiteinclude_HEADERS = \
        ctest.h \
        csuite.h

noinst_PROGRAMS = testcomplex

testcomplex_SOURCES = testcomplex.c
testcomplex_LDFLAGS = -g
testcomplex_DEPENDENCIES = libtestsuite.la
testcomplex_LDADD = libtestsuite.la -lm

tests: testcomplex
