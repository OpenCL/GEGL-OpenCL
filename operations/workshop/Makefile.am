SUBDIRS = generated external

include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

BUILT_SOURCES = $(subst .c,.c.h,$(wildcard $(srcdir)/*.c))

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES =    \
	buffer-cache.la \
	component-extract.la \
	color-warp.la \
	demosaic-bimedian.la \
	demosaic-simple.la \
	ditto.la \
	hstack.la \
	ink-simulator.la \
	kuwahara.la \
	mandelbrot.la \
	median-blur.la \
	rawbayer-load.la \
	rgb-clip.la \
	unpremul.la \
	gradient-map.la
