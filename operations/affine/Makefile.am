if OS_WIN32     
mwindows = -mwindows
no_undefined = -no-undefined
libgegl = $(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la
endif

INCLUDES = \
	@DEP_CFLAGS@				\
	@BABL_CFLAGS@				\
	@CPPFLAGS@				\
	-I$(srcdir)				\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-I$(top_srcdir)/gegl			\
	-I$(top_srcdir)/gegl/buffer		\
	-I$(top_srcdir)/gegl/operation		\
	-I$(top_srcdir)/gegl/property-types	\
	-I$(top_srcdir)/gegl/module

affine_ladir = $(DESTDIR)$(libdir)/gegl-@GEGL_API_VERSION@

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
	translate.c	\
	chant.h

LDADD = $(libgegl)

affine_la_LDFLAGS = -module -avoid-version $(no_undefined) $(mwindows) 
