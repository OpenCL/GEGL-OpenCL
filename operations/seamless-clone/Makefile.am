include $(top_srcdir)/operations/Makefile-common.am

###############################
# Seamless cloning operations #
###############################

AM_LDFLAGS += $(P2TC_LDLAGS)
AM_CFLAGS += $(P2TC_CFLAGS) -I$(top_srcdir)/seamless-clone

LIBS = $(op_libs)

# A simple seamless cloning operation
seamless_clone_la_SOURCES = seamless-clone.c
seamless_clone_la_LIBADD = $(top_builddir)/seamless-clone/libgegl-sc-@GEGL_API_VERSION@.la

# A composing operation for using the seamless cloning
seamless_clone_compose_la_SOURCES = seamless-clone-compose.c
seamless_clone_compose_la_LIBADD = $(top_builddir)/seamless-clone/libgegl-sc-@GEGL_API_VERSION@.la

# Install both the operations in the op dir
#
#   TODO: Currently we define this variable manually since it's defined
#   in the operations automake file, but there it comes together with
#   other definitions that we don't want...
opdir = $(libdir)/gegl-@GEGL_API_VERSION@
op_LTLIBRARIES = seamless-clone.la seamless-clone-compose.la
