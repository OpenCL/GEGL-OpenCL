SUBDIRS = poly2tri-c

include $(top_srcdir)/operations/Makefile-common.am

LIBS            = $(op_libs)

seamless_clone_la_SOURCES = \
	make-mesh.c			\
	make-mesh.h			\
	find-outline.c		\
	find-outline.h		\
	seamless-clone.c	\
	seamless-clone.h

seamless_clone_la_LIBADD = $(op_libs) poly2tri-c/libpoly2tri-c.la
seamless_clone_la_CFLAGS = $(AM_CFLAGS) $(BABL_CFLAGS) $(GLIB_CFLAGS)

opdir = $(libdir)/gegl-@GEGL_API_VERSION@
op_LTLIBRARIES = seamless-clone.la

AM_CPPFLAGS += \
	-I$(top_srcdir)/operations/common
