SUBDIRS = generated external

include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

BUILT_SOURCES = $(subst .c,.c.h,$(wildcard $(srcdir)/*.c))

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES =    \
	bayer-matrix.la \
	bilateral-filter-fast.la \
	demosaic-bimedian.la \
	demosaic-simple.la \
	ditto.la \
	gradient-map.la \
	hstack.la \
	integral-image.la \
	kuwahara.la \
	linear-sinusoid.la \
	mandelbrot.la \
	median-blur.la \
	rawbayer-load.la \
	segment-kmeans.la \
	shadows-highlights.la \
	shadows-highlights-correction.la \
