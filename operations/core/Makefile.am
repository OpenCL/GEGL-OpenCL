include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES = \
	cache.la \
	clone.la \
	cast-format.la \
	convert-format.la \
	crop.la \
	json.la \
	nop.la
