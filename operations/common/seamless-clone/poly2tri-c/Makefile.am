SUBDIRS = \
	common		\
	sweep		\
	refine		\
	render

noinst_LTLIBRARIES = libpoly2tri-c.la

libpoly2tri_c_la_SOURCES = poly2tri.h
libpoly2tri_c_la_LIBADD  = \
	common/libpoly2tri-c-common.la	\
	sweep/libpoly2tri-c-sweep.la	\
	refine/libpoly2tri-c-refine.la	\
	render/libpoly2tri-c-render.la
libpoly2tri_c_la_CFLAGS  = $(DEP_CFLAGS) -fPIC
