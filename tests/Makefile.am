# The tests
TESTS =   \
	test-proxynop-processing$(EXEEXT)   \
	test-gegl-rectangle$(EXEEXT)
EXTRA_PROGRAMS =   \
	$(TESTS)

# Common CPPFLAGS
AM_CPPFLAGS =   \
	-I$(top_srcdir)   \
	-I$(top_srcdir)/gegl/   \
	-I$(top_srcdir)/gegl/buffer   \
	-I$(top_srcdir)/gegl/property-types   \
	-I$(top_srcdir)/gegl/operation   \
	@DEP_CFLAGS@ @BABL_CFLAGS@

# Proxynop Processing
test_proxynop_processing_SOURCES = \
	test-proxynop-processing.c
test_proxynop_processing_LDADD = \
	$(top_builddir)/gegl/libgegl-@GEGL_API_VERSION@.la \
	@DEP_LIBS@ @BABL_LIBS@

# GeglRectangle
test_gegl_rectangle_SOURCES =   \
	test-gegl-rectangle.c
test_gegl_rectangle_LDADD =   \
	$(top_builddir)/gegl/libgegl-@GEGL_API_VERSION@.la   \
	@DEP_LIBS@ @BABL_LIBS@
