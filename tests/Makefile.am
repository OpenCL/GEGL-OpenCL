SUBDIRS = buffer

# Make the tests run against the build and not the installation
TESTS_ENVIRONMENT = \
	GEGL_PATH=$(top_builddir)/operations/common:$(top_builddir)/operations/core

# The tests
TESTS = \
	test-proxynop-processing$(EXEEXT)	\
	test-gegl-rectangle$(EXEEXT)

noinst_PROGRAMS = $(TESTS)

# Common CPPFLAGS
AM_CPPFLAGS = \
	-I$(top_srcdir)				\
	-I$(top_srcdir)/gegl			\
	-I$(top_srcdir)/gegl/buffer		\
	-I$(top_srcdir)/gegl/property-types	\
	-I$(top_srcdir)/gegl/operation		\
	@DEP_CFLAGS@ @BABL_CFLAGS@

# Common libs
testlibs = \
	$(top_builddir)/gegl/libgegl-@GEGL_API_VERSION@.la	\
	@DEP_LIBS@ @BABL_LIBS@

# Proxynop Processing
test_proxynop_processing_SOURCES = test-proxynop-processing.c
test_proxynop_processing_LDADD = $(testlibs)

# GeglRectangle
test_gegl_rectangle_SOURCES = test-gegl-rectangle.c
test_gegl_rectangle_LDADD = $(testlibs)
