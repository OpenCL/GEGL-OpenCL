SUBDIRS = data

# The tests
noinst_PROGRAMS = \
	test-save \
	test-load \
	test-roundtrip

TESTS = $(noinst_PROGRAMS)

# Included directly in other c files so it needs to be disted
EXTRA_DIST = common.c

# Common CPPFLAGS
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/graph \
	-I$(top_srcdir)/gegl/graph \
	-I$(top_builddir)/gegl/operation \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl \
	-I$(top_builddir)/gegl/process \
	-I$(top_srcdir)/gegl/process \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS)

# Common libs
LIBS = $(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la	\
	$(DEP_LIBS) $(BABL_LIBS) $(MATH_LIB)

check-TESTS: $(TESTS)
	$(PYTHON) $(srcdir)/../run-tests.py \
	  --build-dir=$(top_builddir) --src-dir=$(top_srcdir) \
	  $(TESTS)
