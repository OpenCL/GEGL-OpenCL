INCLUDES = $(PYTHON_INCLUDES) $(PYGOBJECT_CFLAGS) $(GEGL_CFLAGS)

AM_CFLAGS = $(PYGEGL_EXTRA_CFLAGS)

pygegldir = $(pyexecdir)/Gegl
pygegl_PYTHON = __init__.py fifthleg.py

pygegl_LTLIBRARIES = _gegl.la

_gegl_la_LDFLAGS = -module -avoid-version -export-symbols-regex init_gegl
_gegl_la_LIBADD = $(GEGL_LIBS) $(PYTHON_LIBS)
_gegl_la_SOURCES = geglmodule.c

nodist__gegl_la_SOURCES = gegl.c

CLEANFILES = gegl.c
EXTRA_DIST = gegl.defs gegl.override geglrectangle.override

gegl.c: gegl.defs gegl.override geglrectangle.override

.defs.c:
	(cd $(srcdir) \
	 && $(PYGTK_CODEGEN) \
	    --py_ssize_t-clean \
	    --override $*.override \
	    --prefix py$* $*.defs) > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c
