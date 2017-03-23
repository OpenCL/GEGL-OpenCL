include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.rb) $(wildcard $(srcdir)/*.h)

AM_CPPFLAGS += -I$(srcdir)
AM_CFLAGS += -DGEGL_OP_BUNDLE

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES = \
  gegl-generated.la

gegl_generated_la_SOURCES = \
  add.c \
  clear.c \
  color-burn.c \
  color-dodge.c \
  darken.c \
  difference.c \
  divide.c \
  dst-atop.c \
  dst-in.c \
  dst-out.c \
  dst-over.c \
  dst.c \
  exclusion.c \
  gamma.c \
  hard-light.c \
  lighten.c \
  multiply.c \
  overlay.c \
  plus.c \
  screen.c \
  soft-light.c \
  src-atop.c \
  src-in.c \
  src-out.c \
  src.c \
  subtract.c \
  xor.c \
  module.c

module.c: $(gegl_generated_la_SOURCES)
	$(top_srcdir)/tools/gen-loader.sh $^ > module.c
