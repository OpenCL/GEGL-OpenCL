SUBDIRS = sweep common

noinst_LTLIBRARIES = libp2tc.la

AM_CFLAGS = $(GLIB_CFLAGS) $(P2TC_CFLAGS)
AM_LDFLAGS = $(GLIB_LIBS)

libp2tc_la_LIBADD = common/libp2tc-common.la sweep/libp2tc-sweep.la
libp2tc_la_SOURCES = poly2tri.h
