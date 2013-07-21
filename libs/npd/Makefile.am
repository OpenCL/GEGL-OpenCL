noinst_LTLIBRARIES = libnpd.la

AM_CFLAGS = $(GLIB_CFLAGS) $(NPD_CFLAGS) -I$(top_builddir)/gegl -I$(top_srcdir)/gegl
AM_LDFLAGS = $(GLIB_LIBS) $(NPD_LIBS)

libnpd_la_SOURCES = \
    npd_common.h    \
    npd_common.c    \
    deformation.h   \
    deformation.c   \
    npd_math.h      \
    npd_math.c      \
    graphics.h      \
    graphics.c      \
    npd_gegl.h      \
    npd_gegl.c
