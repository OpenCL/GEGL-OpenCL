SUBDIRS = generated

include $(top_srcdir)/operations/Makefile-operations.am

line-profile.so: line-profile.c $(GEGLHEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)

stress.so: stress.c $(GEGLHEADERS) envelopes.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)
c2g.so: c2g.c $(GEGLHEADERS) envelopes.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)
min-envelope.so: min-envelope.c $(GEGLHEADERS) envelopes.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)
max-envelope.so: max-envelope.c $(GEGLHEADERS) envelopes.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)

lum-profile.so: lum-profile.c $(GEGLHEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)
gluas.so: gluas.c $(GEGLHEADERS)
	-$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) `lua-config --libs --include`

gst.so: gst.c $(GEGLHEADERS)
	-$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) `pkg-config gstreamer-0.10 --libs --cflags`

ff-load.so: ff-load.c $(GEGLHEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) `pkg-config libavcodec libavformat --libs --cflags`

ff-save.so: ff-save.c $(GEGLHEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) `pkg-config libavcodec libavformat --libs --cflags`

gtk-display.so: gtk-display.c $(GEGLHEADERS)
	@[ ! -z "$(GTK_LIBS)" ] &&\
	(echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(GTK_CFLAGS) $(GTK_LIBS))\
	||true

save-pixbuf.so: save-pixbuf.c $(GEGLHEADERS)
	@[ ! -z "$(GTK_LIBS)" ] &&\
	(echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) `pkg-config gdk-pixbuf-2.0 --libs --cflags`)\
	||true

distcheck:
	@:
