# Make the tests run against the build and not the installation
TESTS_ENVIRONMENT = \
	GEGL_PATH=$(top_builddir)/operations/
	ABS_TOP_BUILDDIR=$(top_builddir) \
	ABS_TOP_SRCDIR=$(top_srcdir)

# The tests
noinst_PROGRAMS = \
	test-change-processor-rect	\
	test-gegl-tile			\
	test-color-op			\
	test-gegl-rectangle		\
	test-misc			\
	test-path			\
	test-proxynop-processing

EXTRA_DIST = test-exp-combine.sh

TESTS = $(noinst_PROGRAMS) test-exp-combine.sh

# Common CPPFLAGS
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types \
	-I$(top_builddir)/gegl/operation \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS)

# Common libs
LIBS = $(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la	\
	$(DEP_LIBS) $(BABL_LIBS)
