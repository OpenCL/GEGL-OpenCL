include $(top_srcdir)/operations/Makefile-common.am

LIBS = $(op_libs)

transformopslibdir = $(libdir)/gegl-$(GEGL_API_VERSION)

transformopslib_LTLIBRARIES = transformops.la

transformops_la_SOURCES = \
	transform-core.c	\
	transform-core.h	\
	module.c	\
	module.h	\
	reflect.c	\
	rotate.c	\
	rotate-on-center.c      \
	scale-ratio.c \
	scale-size.c \
	scale-size-keepaspect.c \
	shear.c		\
	transform.c	\
	translate.c
