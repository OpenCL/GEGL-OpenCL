include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.rb) $(wildcard $(srcdir)/*.h)

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES = \
  add.la \
  clear.la \
  color-burn.la \
  color-dodge.la \
  darken.la \
  difference.la \
  divide.la \
  dst-atop.la \
  dst-in.la \
  dst-out.la \
  dst-over.la \
  dst.la \
  exclusion.la \
  gamma.la \
  hard-light.la \
  lighten.la \
  multiply.la \
  overlay.la \
  plus.la \
  screen.la \
  soft-light.la \
  src-atop.la \
  src-in.la \
  src-out.la \
  src.la \
  subtract.la \
  svg-multiply.la \
  xor.la