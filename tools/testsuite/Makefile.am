
lib_LTLIBRARIES = libtestsuite.la

libtestsuite_la_SOURCES = \
	ctest.c		\
        ctest.h		\
	csuite.c	\
        csuite.h


TESTS = testcomplex

testcomplex_SOURCES = testcomplex.c
testcomplex_LDFLAGS = -g
testcomplex_DEPENDENCIES = libtestsuite.la
testcomplex_LDADD = libtestsuite.la -lm

EXTRA_PROGRAMS = testcomplex
