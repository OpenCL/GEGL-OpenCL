
noinst_PROGRAMS = format pipeline buffer

BUILT_SOURCES = \
	$(noinst_PROGRAMS:=.vala.stamp)

# BABL
format_VALASOURCES = format.vala
format_VALAPKGS = --vapidir=$(top_srcdir) --pkg=babl

format_SOURCES = \
	$(format_VALASOURCES:.vala=.c) \
	$(format_VALASOURCES:.vala=.h)

format_CFLAGS = \
	$(VALA_TEST_CFLAGS)
format_LDFLAGS = \
	$(VALA_TEST_LIBS) \
	-lbabl-0.0

format.vala.stamp: $(format_VALASOURCES)
	$(VALAC) -C $(format_VALAPKGS) $^
	touch $@

# PIPELINE
pipeline_VALASOURCES = pipeline.vala
pipeline_VALAPKGS = \
	--pkg=glib-2.0	\
	--vapidir=$(top_srcdir) --pkg=babl --pkg=gegl

pipeline_SOURCES = \
	$(pipeline_VALASOURCES:.vala=.c)	\
	$(pipeline_VALASOURCES:.vala=.h)

pipeline_CFLAGS = \
	$(VALA_TEST_CFLAGS)
pipeline_LDFLAGS = \
	$(VALA_TEST_LIBS) \
	-lbabl-0.0 -lgegl-0.0

pipeline.vala.stamp: $(pipeline_VALASOURCES)
	$(VALAC) -C $(pipeline_VALAPKGS) $^
	touch $@

# BUFFER
buffer_VALASOURCES = buffer.vala
buffer_VALAPKGS = \
	--pkg=glib-2.0	\
	--vapidir=$(top_srcdir) --pkg=babl --pkg=gegl

buffer_SOURCES = \
	$(buffer_VALASOURCES:.vala=.c)	\
	$(buffer_VALASOURCES:.vala=.h)

buffer_CFLAGS = \
	$(VALA_TEST_CFLAGS)
buffer_LDFLAGS = \
	$(VALA_TEST_LIBS) \
	-lbabl-0.0 -lgegl-0.0

buffer.vala.stamp: $(buffer_VALASOURCES)
	$(VALAC) -C $(buffer_VALAPKGS) $^
	touch $@


EXTRA_DIST = \
	$(format_VALASOURCES)	\
	$(pipeline_VALASOURCES)	\
	$(buffer_VALASOURCES)
