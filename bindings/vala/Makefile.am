SUBDIRS = samples

# babl.vapi: $(top_srcdir)/packages/babl/babl*
# 	vapigen --library=babl $^

# gegl.vapi: $(top_srcdir)/packages/gegl/gegl*
# 	vapigen --library=gegl --vapidir=. --pkg=babl --pkg=glib-2.0 $^

vapidir=$(DESTDIR)@vapidir@
dist_vapi_DATA = \
	babl-0.0.vapi	\
	gegl-0.0.vapi	gegl-0.0.deps
