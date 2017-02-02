SUBDIRS = generated external

include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

BUILT_SOURCES = $(subst .c,.c.h,$(wildcard $(srcdir)/*.c))

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES =    \
	bilateral-filter-fast.la \
	component-extract.la \
	demosaic-bimedian.la \
	demosaic-simple.la \
	ditto.la \
	gradient-map.la \
	hstack.la \
	image-gradient.la \
	integral-image.la \
	kuwahara.la \
	mandelbrot.la \
	median-blur.la \
	rawbayer-load.la \
	segment-kmeans.la \
	unpremul.la \
    vhsfix.la \
    waterpixels.la \
    watershed-transform.la

