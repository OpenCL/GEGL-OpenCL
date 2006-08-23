#SUBDIRS = testsuite

EXTRA_DIST = xml_insert.sh

noinst_PROGRAMS = introspect

introspect_SOURCES = introspect.c

INCLUDES = -I$(top_srcdir)/gegl @DEP_CFLAGS@ @BABL_CFLAGS@
AM_LDFLAGS =  @DEP_LIBS@ @BABL_LIBS@

introspect_LDFLAGS = $(top_builddir)/gegl/libgegl-1.0.la
	
