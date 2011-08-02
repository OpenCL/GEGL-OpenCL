include $(top_srcdir)/operations/Makefile-operations.am

ops = seamless-clone.la

seamless_clone_la_SOURCES = \
	make-mesh.c			\
	make-mesh.h			\
	find-outline.c		\
	find-outline.h		\
	seamless-clone.c	\
	seamless-clone.h

seamless_clone_la_LIBADD = $(op_libs) poly2tri-c/poly2tri-c.la
seamless_clone_CFLAGS = $(AM_CFLAGS)

opdir = $(libdir)/gegl-@GEGL_API_VERSION@
op_LTLIBRARIES = $(ops)

AM_CPPFLAGS += \
	-I$(top_srcdir)/operations/common
