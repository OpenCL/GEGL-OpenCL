AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl

AM_CFLAGS = $(DEP_CFLAGS) $(BABL_CFLAGS)

noinst_LTLIBRARIES = libbuffer.la

libbuffer_la_SOURCES = \
    gegl-tile-backend-tiledir.c	\
    gegl-buffer.c		\
    gegl-buffer-access.c	\
    gegl-buffer-share.c		\
    gegl-buffer-index.h		\
    gegl-buffer-iterator.c	\
    gegl-buffer-cl-iterator.c	\
    gegl-buffer-cl-cache.c	\
    gegl-buffer-linear.c	\
    gegl-buffer-save.c		\
    gegl-buffer-load.c		\
    gegl-cache.c		\
    gegl-sampler.c		\
    gegl-sampler-cubic.c	\
    gegl-sampler-linear.c	\
    gegl-sampler-nearest.c	\
    gegl-sampler-lohalo.c       \
    gegl-region-generic.c	\
    gegl-tile.c			\
    gegl-tile-source.c		\
    gegl-tile-storage.c		\
    gegl-tile-backend.c		\
    gegl-tile-backend-file.c	\
    gegl-tile-backend-ram.c	\
    gegl-tile-handler.c		\
    gegl-tile-handler-cache.c	\
    gegl-tile-handler-chain.c	\
    gegl-tile-handler-empty.c	\
    gegl-tile-handler-log.c	\
    gegl-tile-handler-zoom.c	\
    gegl-id-pool.c		\
    \
    gegl-buffer.h		\
    gegl-buffer-private.h	\
    gegl-buffer-iterator.h	\
    gegl-buffer-cl-iterator.h	\
    gegl-buffer-cl-cache.h	\
    gegl-buffer-load.h		\
    gegl-buffer-save.h		\
    gegl-buffer-types.h		\
    gegl-cache.h		\
    gegl-sampler.h		\
    gegl-sampler-cubic.h	\
    gegl-sampler-linear.h	\
    gegl-sampler-nearest.h	\
    gegl-sampler-lohalo.h       \
    gegl-region.h		\
    gegl-region-generic.h	\
    gegl-tile.h			\
    gegl-tile-source.h		\
    gegl-tile-storage.h		\
    gegl-tile-backend.h		\
    gegl-tile-backend-file.h	\
    gegl-tile-backend-tiledir.h	\
    gegl-tile-backend-ram.h	\
    gegl-tile-handler.h		\
    gegl-tile-handler-chain.h	\
    gegl-tile-handler-cache.h	\
    gegl-tile-handler-empty.h	\
    gegl-tile-handler-log.h	\
    gegl-tile-handler-zoom.h	\
    gegl-id-pool.h
