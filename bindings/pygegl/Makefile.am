AUTOMAKE_OPTIONS = dist-bzip2

INCLUDES = $(PYTHON_INCLUDES) $(PYGOBJECT_CFLAGS) $(GEGL_CFLAGS) $(GTK_CFLAGS)

SUBDIRS = tests

# gegl module
pyexec_LTLIBRARIES = geglmodule.la

geglmodule_la_LDFLAGS = -module -avoid-version -export-symbols-regex initgegl $(GTK_LIBS)
geglmodule_la_LIBADD = $(GEGL_LIBS) $(PYGOBJECT_LIBS)
geglmodule_la_SOURCES = geglmodule.c

nodist_geglmodule_la_SOURCES = gegl.c

gegl.c: gegl.defs gegl.override generate_api_bindings.sh

.defs.c:
	(cd $(srcdir)\
	 && $(PYTHON) $(PYGOBJECT_CODEGENDIR)/codegen.py \
	    --override $*.override \
	    --prefix py$* $*.defs) > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c

CLEANFILES = gegl.c

EXTRA_DIST = \
	gegl.defs		\
	gegl.override		\
	generate_api_bindings.sh

#	tests/run_tests		\
#	tests/*.py		\
#	tests/images/*.jpg	\
#	tests/images/*.png
