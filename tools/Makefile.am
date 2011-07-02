#SUBDIRS = testsuite

EXTRA_DIST = \
	xml_insert.sh \
	create-reference.rb \
	gobj2dot.rb

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/module \
	-I$(top_srcdir)/gegl/module \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS)

AM_LDFLAGS = \
	$(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la \
	$(DEP_LIBS) $(BABL_LIBS)

noinst_PROGRAMS = introspect operation_reference img_cmp

if HAVE_EXIV2
noinst_PROGRAMS     += exp_combine 
exp_combine_SOURCES  = exp_combine.cpp
exp_combine_LDADD    = $(EXIV2_LIBS) 
exp_combine_CXXFLAGS = $(AM_CFLAGS) $(EXIV2_CFLAGS)
endif
