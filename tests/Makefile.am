SUBDIRS = buffer compositions

# Make the tests run against the build and not the installation
TESTS_ENVIRONMENT = \
	GEGL_PATH=$(top_builddir)/operations/common:$(top_builddir)/operations/core:$(top_builddir)/operations/external:$(top_builddir)/operations/affine:$(top_builddir)/operations/generated

# The tests
TESTS = \
	test-change-processor-rect	\
	test-proxynop-processing	\
	test-color-op			\
	test-gegl-rectangle
noinst_PROGRAMS = $(TESTS)

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
	-I$(top_srcdir)/gegl/operation

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS)

# Common libs
LIBS = $(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la	\
	$(DEP_LIBS) $(BABL_LIBS)
