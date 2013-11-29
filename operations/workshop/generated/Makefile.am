include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.rb) $(wildcard $(srcdir)/*.h)

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES = \
	average.la \
	blend-reflect.la \
	negation.la \
	soft-burn.la \
	soft-dodge.la \
	subtractive.la