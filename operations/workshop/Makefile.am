SUBDIRS = generated

include $(top_srcdir)/operations/Makefile-operations.am

CFLAGS += -I/home/pippin/src/gimp/app -I/home/pippin/src/gimp 

line-profile$(SHREXT): line-profile.c $(GEGLHEADERS)
	-$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)

stress$(SHREXT): stress.c $(GEGLHEADERS) envelopes.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)
c2g$(SHREXT): c2g.c $(GEGLHEADERS) envelopes.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)
min-envelope$(SHREXT): min-envelope.c $(GEGLHEADERS) envelopes.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)
max-envelope$(SHREXT): max-envelope.c $(GEGLHEADERS) envelopes.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)

lum-profile$(SHREXT): lum-profile.c $(GEGLHEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(CAIRO_CFLAGS) $(CAIRO_LIBS)

gluas$(SHREXT): gluas.c $(GEGLHEADERS)
	@if [ ! -z "$(LUA_LIBS)" ]; then \
	echo $@;\
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(LUA_CFLAGS) $(LUA_LIBS);\
	fi

gst$(SHREXT): gst.c $(GEGLHEADERS)
	-$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) `pkg-config gstreamer-0.10 --libs --cflags`

ff-save$(SHREXT): ff-save.c $(GEGLHEADERS)
	-$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) \
		$(AVCODEC_CFLAGS) $(AVCODEC_LIBS) \
		$(AVFORMAT_CFLAGS) $(AVFORMAT_LIBS) 

gtk-display$(SHREXT): gtk-display.c $(GEGLHEADERS)
	@[ ! -z "$(GTK_LIBS)" ] &&\
	(echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(GTK_CFLAGS) $(GTK_LIBS))\
	||true
