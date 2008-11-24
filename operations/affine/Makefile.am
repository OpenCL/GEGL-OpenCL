include $(top_srcdir)/operations/Makefile-common.am

affine_ladir = $(libdir)/gegl-@GEGL_API_VERSION@

affine_la_LTLIBRARIES = affine.la

affine_la_SOURCES = \
	affine.c	\
	affine.h	\
	matrix.c	\
	matrix.h	\
	module.c	\
	module.h	\
	reflect.c	\
	rotate.c	\
	scale.c		\
	shear.c		\
	transform.c	\
	translate.c	\
	chant.h

affine_la_LIBADD = $(op_libs)
