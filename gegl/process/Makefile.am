noinst_LTLIBRARIES = libprocess.la

PROCESS_sources = 			\
	gegl-cr-visitor.c		\
	gegl-debug-rect-visitor.c	\
	gegl-eval-mgr.c			\
	gegl-eval-visitor.c		\
	gegl-finish-visitor.c		\
	gegl-have-visitor.c		\
	gegl-need-visitor.c		\
	gegl-prepare-visitor.c		\
	gegl-processor.c



PROCESS_headers = 			\
	gegl-cr-visitor.h		\
	gegl-debug-rect-visitor.h	\
	gegl-eval-mgr.h			\
	gegl-eval-visitor.h		\
	gegl-finish-visitor.h		\
	gegl-have-visitor.h		\
	gegl-need-visitor.h		\
	gegl-prepare-visitor.h		\
	gegl-processor.h


libprocess_la_SOURCES = $(PROCESS_sources) $(PROCESS_headers)
#libprocessinclude_HEADERS = #
#libprocessincludedir = $(includedir)/gegl-$(GEGL_API_VERSION)/gegl/process

INCLUDES = \
	   -I$(top_srcdir)		\
	   -I$(top_srcdir)/gegl		\
	   -I$(top_srcdir)/gegl/buffer	\
	   @DEP_CFLAGS@ @BABL_CFLAGS@
