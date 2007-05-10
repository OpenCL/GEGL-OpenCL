noinst_LTLIBRARIES = libbuffer.la

BUFFER_sources = \
    gegl-buffer-allocator.c	\
    gegl-buffer-save.c		\
    gegl-buffer-load.c		\
    gegl-buffer.c		\
    gegl-cache.c		\
    gegl-interpolator.c		\
    gegl-interpolator-cubic.c	\
    gegl-interpolator-lanczos.c	\
    gegl-interpolator-linear.c	\
    gegl-interpolator-nearest.c	\
    gegl-region-generic.c	\
    gegl-storage.c		\
    gegl-tile.c			\
    gegl-tile-backend.c		\
    gegl-tile-cache.c		\
    gegl-tile-disk.c		\
    gegl-tile-empty.c		\
    gegl-tile-log.c		\
    gegl-tile-mem.c		\
    gegl-tile-store.c		\
    gegl-tile-trait.c		\
    gegl-tile-traits.c		\
    gegl-tile-zoom.c

BUFFER_headers = \
    gegl-buffer-allocator.h	\
    gegl-buffer.h		\
    gegl-buffer-save.h		\
    gegl-buffer-types.h		\
    gegl-cache.h		\
    gegl-interpolator.h		\
    gegl-interpolator-cubic.h	\
    gegl-interpolator-lanczos.h	\
    gegl-interpolator-linear.h	\
    gegl-interpolator-nearest.h	\
    gegl-region.h		\
    gegl-region-generic.h	\
    gegl-storage.h		\
    gegl-tile.h			\
    gegl-tile-backend.h		\
    gegl-tile-cache.h		\
    gegl-tile-disk.h		\
    gegl-tile-empty.h		\
    gegl-tile-log.h		\
    gegl-tile-mem.h		\
    gegl-tile-store.h		\
    gegl-tile-trait.h		\
    gegl-tile-traits.h		\
    gegl-tile-zoom.h

libbuffer_la_SOURCES = $(BUFFER_sources) $(BUFFER_headers)
libbufferinclude_HEADERS = gegl-buffer.h

libbufferincludedir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl/buffer

INCLUDES = \
	   -I$(top_srcdir)	\
	   -I$(top_srcdir)/gegl	\
	   @DEP_CFLAGS@ @BABL_CFLAGS@

