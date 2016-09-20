#SUBDIRS = testsuite

EXTRA_DIST = \
	xml_insert.sh \
    gen-loader.sh \
    csourcetostring.py \
	gobj2dot.rb

AM_CPPFLAGS = \
	-DTOP_SRCDIR=\"$(top_srcdir)\" \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/graph \
	-I$(top_srcdir)/gegl/graph \
	-I$(top_builddir)/gegl/module \
	-I$(top_srcdir)/gegl/module \
	-I$(top_builddir)/gegl/operation \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl \
	-I$(top_builddir)/gegl/process \
	-I$(top_srcdir)/gegl/process \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS)

AM_LDFLAGS = \
	$(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la \
	$(DEP_LIBS) $(BABL_LIBS) $(MATH_LIB)

bin_PROGRAMS = gegl-imgcmp
noinst_PROGRAMS = introspect operation_reference detect_opencl gegl-tester

gegl_tester_SOURCES = \
	gegl-tester.c

if HAVE_EXIV2
noinst_PROGRAMS     += exp_combine 
exp_combine_SOURCES  = exp_combine.cpp
exp_combine_LDADD    = $(EXIV2_LIBS) 
exp_combine_CXXFLAGS = $(AM_CFLAGS) $(EXIV2_CFLAGS)
endif
