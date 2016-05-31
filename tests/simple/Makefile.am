# The tests
noinst_PROGRAMS =			\
	test-backend-file		\
	test-buffer-cast		\
	test-buffer-changes		\
	test-buffer-extract		\
	test-buffer-hot-tile	\
	test-buffer-tile-voiding	\
	test-change-processor-rect	\
	test-convert-format		\
	test-color-op			\
	test-empty-tile			\
	test-format-sensing		\
	test-gegl-rectangle		\
	test-gegl-color		    \
	test-gegl-tile			\
	test-image-compare		\
	test-license-check		\
	test-misc			\
	test-node-connections		\
	test-node-properties		\
	test-object-forked		\
	test-opencl-colors		\
	test-serialize \
	test-path			\
	test-proxynop-processing	\
	test-scaled-blit		\
	test-svg-abyss

EXTRA_DIST = test-exp-combine.sh

TESTS = $(noinst_PROGRAMS) test-exp-combine.sh

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

clean-local:
	rm -f test-exp-combine.hdr test-exp-combine-diff.png
