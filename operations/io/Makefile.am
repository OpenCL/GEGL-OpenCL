include $(top_srcdir)/operations/Makefile-operations.am

CXXFILES         = $(wildcard $(srcdir)/*.cpp)
SOBJS           += $(subst $(srcdir)/,,$(CXXFILES:.cpp=$(SHREXT)))
INSTALLED_ITEMS += $(subst $(srcdir),$(ext_dir),$(CXXFILES:.cpp=$(SHREXT)))
EXTRA_DIST      += $(wildcard *.cpp)



ff-load$(SHREXT): ff-load.c $(GEGLHEADERS)
	@if [ ! -z "$(AVCODEC_LIBS)" ]; then \
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) \
		$(AVCODEC_CFLAGS) $(AVCODEC_LIBS) \
		$(AVFORMAT_CFLAGS) $(AVFORMAT_LIBS);\
	fi

jpg-load$(SHREXT): jpg-load.c $(GEGLHEADERS)
	@if [ ! -z "$(LIBJPEG)" ]; then \
	echo $@;\
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(LIBJPEG);\
	fi

png-load$(SHREXT): png-load.c $(GEGLHEADERS)
	@echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(PNG_CFLAGS) $(PNG_LIBS)

png-save$(SHREXT): png-save.c $(GEGLHEADERS)
	@echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(PNG_CFLAGS) $(PNG_LIBS)

svg-load$(SHREXT): svg-load.c $(GEGLHEADERS)
	@if [ ! -z "$(RSVG_LIBS)" ]; then \
	echo $@;\
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) \
                                     $(CAIRO_CFLAGS) $(CAIRO_LIBS) \
                                     $(RSVG_CFLAGS) $(RSVG_LIBS);\
	fi

pixbuf$(SHREXT): pixbuf.c $(GEGLHEADERS)
	@if [ ! -z "$(RSVG_LIBS)" ]; then \
	echo $@;\
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(RSVG_CFLAGS) $(RSVG_LIBS);\
	fi

exr-load$(SHREXT): exr-load.cpp $(GEGLHEADERS)
	@if [ ! -z "$(OPENEXR_LIBS)" ]; then \
	echo $@;\
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(OPENEXR_CFLAGS) $(OPENEXR_LIBS);\
	fi

save-pixbuf$(SHREXT): save-pixbuf.c $(GEGLHEADERS)
	@[ ! -z "$(GTK_LIBS)" ] &&\
	(echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) `pkg-config gdk-pixbuf-2.0 --libs --cflags`)\
	||true

display$(SHREXT): display.c $(GEGLHEADERS)
	@[ ! -z "$(SDL_LIBS)" ] &&\
	(echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(SDL_CFLAGS) $(SDL_LIBS)) \
	|| true

convert-format$(SHRECT): convert-format.c $(GEGLHEADERS)
	@(echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $a $< $(LDADD))