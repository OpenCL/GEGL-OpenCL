SUBDIRS = poly2tri-c

include $(top_srcdir)/operations/Makefile-common.am

AM_LDFLAGS += $(op_libs) poly2tri-c/libpoly2tri-c.la

sc_common_files =		\
 	make-mesh.c		\
	make-mesh.h		\
	find-outline.c		\
	find-outline.h		\
	seamless-clone-common.c	\
	seamless-clone-common.h

seamless_clone_la_SOURCES = seamless-clone.c $(sc_common_files)
seamless_clone_render_la_SOURCES = seamless-clone-render.c $(sc_common_files)
seamless_clone_prepare_la_SOURCES = seamless-clone-prepare.c $(sc_common_files)

opdir = $(libdir)/gegl-@GEGL_API_VERSION@

op_LTLIBRARIES = seamless-clone.la seamless-clone-render.la seamless-clone-prepare.la
