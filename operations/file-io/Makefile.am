include $(top_srcdir)/operations/Makefile-operations.am

CXXFILES         = $(wildcard $(srcdir)/*.cpp)
SOBJS           += $(subst $(srcdir)/,,$(CXXFILES:.cpp=$(SHREXT)))
INSTALLED_ITEMS += $(subst $(srcdir),$(ext_dir),$(CXXFILES:.cpp=$(SHREXT)))
EXTRA_DIST      += $(wildcard *.cpp)


# If extensions needing external libraries are to be compiled with this make
# file, each of them can be added according to this pattern:
# extra.so: extra.c
#	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<  [own compile and link flags]

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
