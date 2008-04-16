noinst_LTLIBRARIES = libbuffer.la

BUFFER_sources = \
    gegl-buffer-save.c		\
    gegl-buffer-load.c		\
    gegl-buffer.c		\
    gegl-cache.c		\
    gegl-sampler.c		\
    gegl-sampler-cubic.c	\
    gegl-sampler-lanczos.c	\
    gegl-sampler-linear.c	\
    gegl-sampler-nearest.c	\
    gegl-region-generic.c	\
    gegl-tile.c			\
    gegl-tile-source.c		\
    gegl-tile-storage.c		\
    gegl-tile-backend.c		\
    gegl-tile-backend-swapfile.c	\
    gegl-tile-backend-gio-tiles.c	\
    gegl-tile-backend-ram.c	\
    gegl-tile-handler.c		\
    gegl-tile-handler-chain.c	\
    gegl-tile-handler-cache.c	\
    gegl-tile-handler-empty.c	\
    gegl-tile-handler-log.c	\
    gegl-tile-handler-zoom.c	\
    gegl-id-pool.c

BUFFER_headers = \
    gegl-buffer.h		\
    gegl-buffer-private.h	\
    gegl-buffer-save.h		\
    gegl-buffer-types.h		\
    gegl-cache.h		\
    gegl-sampler.h		\
    gegl-sampler-cubic.h	\
    gegl-sampler-lanczos.h	\
    gegl-sampler-linear.h	\
    gegl-sampler-nearest.h	\
    gegl-region.h		\
    gegl-region-generic.h	\
    gegl-tile.h			\
    gegl-tile-source.h		\
    gegl-tile-storage.h		\
    gegl-tile-backend.h		\
    gegl-tile-backend-swapfile.h	\
    gegl-tile-backend-gio-tiles.h	\
    gegl-tile-backend-ram.h		\
    gegl-tile-handler.h		\
    gegl-tile-handler-chain.h		\
    gegl-tile-handler-cache.h	\
    gegl-tile-handler-empty.h	\
    gegl-tile-handler-log.h		\
    gegl-tile-handler-zoom.h		\
    gegl-id-pool.h

libbuffer_la_SOURCES = $(BUFFER_sources) $(BUFFER_headers)

INCLUDES = \
	   -I$(top_srcdir)	\
	   -I$(top_srcdir)/gegl	\
	   @DEP_CFLAGS@ @BABL_CFLAGS@

