include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)
BUILT_SOURCES = $(subst .c,.c.h,$(wildcard $(srcdir)/*.c))

AM_CPPFLAGS += -I$(srcdir)
AM_CFLAGS += -DGEGL_OP_BUNDLE

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES = \
    gegl-core.la

gegl_core_la_SOURCES =\
    module.c \
	cache.c \
	load.c \
	clone.c \
	cast-format.c \
	convert-format.c \
	crop.c \
	json.c \
	nop.c

module.c: $(gegl_core_la_SOURCES)
	$(top_srcdir)/tools/gen-loader.sh $^ > module.c

