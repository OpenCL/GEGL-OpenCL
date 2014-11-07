SUBDIRS = generated external

include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

BUILT_SOURCES = $(subst .c,.c.h,$(wildcard $(srcdir)/*.c))

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES =    \
	box-max.la \
	box-min.la \
	box-percentile.la \
	buffer-cache.la \
	color-warp.la \
	demosaic-bimedian.la \
	demosaic-simple.la \
	disc-percentile.la \
	ditto.la \
	gaussian-blur-iir.la \
	gblur-1d.la \
	hstack.la \
	ink-simulator.la \
	kuwahara.la \
	mandelbrot.la \
	rawbayer-load.la \
	snn-percentile.la \
	unpremul.la \
	gradient-map.la
