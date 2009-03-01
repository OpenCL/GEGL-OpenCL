TESTS = \
	test-proxynop-processing$(EXEEXT)

test_proxynop_processing_SOURCES = \
	test-proxynop-processing.c

test_proxynop_processing_CPPFLAGS = \
	-I$(top_srcdir)				\
	-I$(top_srcdir)/gegl/			\
	-I$(top_srcdir)/gegl/buffer		\
	-I$(top_srcdir)/gegl/property-types	\
	-I$(top_srcdir)/gegl/operation		\
	@DEP_CFLAGS@ @BABL_CFLAGS@

test_proxynop_processing_LDADD = \
	$(top_srcdir)/gegl/libgegl-@GEGL_API_VERSION@.la \
	@DEP_LIBS@ @BABL_LIBS@

EXTRA_PROGRAMS = \
	$(TESTS)
