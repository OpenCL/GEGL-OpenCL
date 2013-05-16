SUBDIRS = p2t refine render

noinst_LTLIBRARIES = libpoly2tri-c.la

AM_CFLAGS = $(GLIB_CFLAGS) $(P2TC_CFLAGS)
AM_LDFLAGS = $(GLIB_LIBS)

libpoly2tri_c_la_LIBADD = p2t/libp2tc.la refine/libp2tc-refine.la render/libp2tc-render.la
libpoly2tri_c_la_SOURCES =
