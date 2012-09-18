include $(top_srcdir)/operations/Makefile-common.am

AM_LDFLAGS += $(op_libs) $(P2TC_LIBS)
AM_CFLAGS  += $(op_cflags) $(P2TC_CFLAGS)

sc_common_files =		\
	sc-common.h		\
	sc-context.c		\
	sc-context.h		\
	sc-context-private.h	\
	sc-outline.c		\
	sc-outline.h		\
	sc-sample.c		\
	sc-sample.h

noinst_LTLIBRARIES = libsc.la
libsc_la_SOURCES = $(sc_common_files)

seamless_clone_la_SOURCES = seamless-clone.c $(sc_common_files)
seamless_clone_render_la_SOURCES = seamless-clone-render.c $(sc_common_files)
seamless_clone_prepare_la_SOURCES = seamless-clone-prepare.c $(sc_common_files)
seamless_clone_compose_la_SOURCES = seamless-clone-compose.c

opdir = $(libdir)/gegl-@GEGL_API_VERSION@

op_LTLIBRARIES = seamless-clone.la seamless-clone-render.la seamless-clone-prepare.la seamless-clone-compose.la
