include $(top_srcdir)/operations/Makefile-common.am

##################################################################
# A shared library for direct access to the seamless cloning API #
##################################################################

GEGL_SC_publicdir = $(includedir)/gegl-$(GEGL_API_VERSION)/sc

GEGL_SC_public_HEADERS = \
	sc-common.h	\
	sc-context.h	\
	sc-outline.h	\
	sc-sample.h

GEGL_SC_SOURCES = \
	$(GEGL_SC_public_HEADERS)	\
	sc-context-private.h		\
	sc-context.c			\
	sc-outline.c			\
	sc-sample.c

libgegl_sc_@GEGL_API_VERSION@_la_SOURCES = \
	$(GEGL_SC_public_HEADERS)	\
	$(GEGL_SC_SOURCES)

libgegl_sc_@GEGL_API_VERSION@_la_LDFLAGS = \
	-avoid-version -export-dynamic $(no_undefined) $(P2TC_LIBS)

libgegl_sc_@GEGL_API_VERSION@_la_CFLAGS = \
  $(AM_CFLAGS) $(P2TC_CFLAGS)

lib_LTLIBRARIES = libgegl-sc-@GEGL_API_VERSION@.la

libgegl_sc_@GEGL_API_VERSION@_la_LIBADD = \
	$(libgegl)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gegl-sc-$(GEGL_API_VERSION).pc

###############################
# Seamless cloning operations #
###############################

AM_LDFLAGS += $(P2TC_LDLAGS)
AM_CFLAGS += $(P2TC_CFLAGS)

LIBS = $(op_libs)

# A simple seamless cloning operation
seamless_clone_la_SOURCES = seamless-clone.c
seamless_clone_la_LIBADD = libgegl-sc-@GEGL_API_VERSION@.la
seamless_clone_la_DEPENDENCIES = libgegl-sc-@GEGL_API_VERSION@.la

# A composing operation for using the seamless cloning
seamless_clone_compose_la_SOURCES = seamless-clone-compose.c
seamless_clone_compose_la_LIBADD = libgegl-sc-@GEGL_API_VERSION@.la
seamless_clone_compose_la_DEPENDENCIES = libgegl-sc-@GEGL_API_VERSION@.la

# Install both the operations in the op dir
#
#   TODO: Currently we define this variable manually since it's defined
#   in the operations automake file, but there it comes together with
#   other definitions that we don't want...
opdir = $(libdir)/gegl-@GEGL_API_VERSION@
op_LTLIBRARIES = seamless-clone.la seamless-clone-compose.la
