include $(top_srcdir)/operations/Makefile-common.am

######################################################
# A shared library for n-point image deformation API #
######################################################

GEGL_NPD_publicdir = $(includedir)/gegl-$(GEGL_API_VERSION)/npd

GEGL_NPD_public_HEADERS = \
	npd_common.h     \
	deformation.h    \
	npd_math.h       \
	graphics.h       \
	lattice_cut.h    \
	npd_gegl.h       \
	npd_debug.h      \
	npd.h

GEGL_NPD_SOURCES = \
	$(GEGL_NPD_public_HEADERS)	\
	npd_common.c			\
	deformation.c			\
	npd_math.c			\
	graphics.c			\
	lattice_cut.c			\
	npd_debug.c			\
	npd_gegl.c

libgegl_npd_@GEGL_API_VERSION@_la_SOURCES = \
	$(GEGL_NPD_public_HEADERS)	\
	$(GEGL_NPD_SOURCES)

libgegl_npd_@GEGL_API_VERSION@_la_LDFLAGS = \
	-avoid-version -export-dynamic $(no_undefined)

libgegl_npd_@GEGL_API_VERSION@_la_CFLAGS = \
  $(AM_CFLAGS)

lib_LTLIBRARIES = libgegl-npd-@GEGL_API_VERSION@.la

libgegl_npd_@GEGL_API_VERSION@_la_LIBADD = \
	$(libgegl) $(GLIB_LIBS) $(BABL_LIBS)
