noinst_LTLIBRARIES = liboperation.la

OPERATION_sources = \
	gegl-extension-handler.c	\
	gegl-operation.c        	\
	gegl-operation-area-filter.c    \
	gegl-operation-composer.c     	\
	gegl-operation-filter.c       	\
	gegl-operation-meta.c       	\
	gegl-operation-point-composer.c \
	gegl-operation-point-filter.c   \
	gegl-operation-sink.c       	\
	gegl-operation-source.c		\
	gegl-operations.c


OPERATION_headers = \
	gegl-extension-handler.h	\
	gegl-operation.h        	\
	gegl-operation-composer.h     	\
	gegl-operation-area-filter.h    \
	gegl-operation-filter.h       	\
	gegl-operation-meta.h       	\
	gegl-operation-point-composer.h \
	gegl-operation-point-filter.h   \
	gegl-operation-sink.h       	\
	gegl-operation-source.h		\
	gegl-operations.c

public_headers = \
	gegl-operation.h        	\
	gegl-operation-composer.h     	\
	gegl-operation-area-filter.h    \
	gegl-operation-filter.h       	\
	gegl-operation-meta.h       	\
	gegl-operation-point-composer.h \
	gegl-operation-point-filter.h   \
	gegl-operation-sink.h       	\
	gegl-operation-source.h


liboperation_la_SOURCES = $(OPERATION_sources) $(public_headers)
liboperationinclude_HEADERS = $(OPERATION_headers)
liboperationincludedir = $(includedir)/gegl-$(GEGL_API_VERSION)/operation

INCLUDES = \
	   -I$(top_srcdir)		\
	   -I$(top_srcdir)/gegl		\
	   -I$(top_srcdir)/gegl/buffer	\
	   @DEP_CFLAGS@ @BABL_CFLAGS@

