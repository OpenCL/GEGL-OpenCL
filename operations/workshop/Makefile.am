SUBDIRS = generated

include $(top_srcdir)/operations/Makefile-operations.am

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
