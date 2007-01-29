#SUBDIRS = testsuite

EXTRA_DIST = xml_insert.sh create-reference.rb

noinst_PROGRAMS = introspect operation_reference

introspect_SOURCES = introspect.c
operation_reference_SOURCES = operation_reference.c

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/gegl @DEP_CFLAGS@ @BABL_CFLAGS@
AM_LDFLAGS =  @DEP_LIBS@ @BABL_LIBS@ $(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la
