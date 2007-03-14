include $(top_srcdir)/operations/Makefile-operations.am

EXTRA_DIST += $(wildcard perlin/*.[ch])

text$(SHREXT): text.c $(GEGLHEADERS)
	@if [ ! -z "$(PANGO_LIBS)" ]; then \
	echo $@;\
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) \
              $(CAIRO_CFLAGS) $(CAIRO_LIBS) \
              $(PANGO_CFLAGS) $(PANGO_LIBS) \
              $(PANGOCAIRO_CFLAGS) $(PANGOCAIRO_LIBS);\
	fi
