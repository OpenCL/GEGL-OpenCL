include $(top_srcdir)/operations/Makefile-operations.am

CXXFILES         = $(wildcard $(srcdir)/*.cpp)
SOBJS           += $(subst $(srcdir)/,,$(CXXFILES:.cpp=.so))
INSTALLED_ITEMS += $(subst $(srcdir),$(ext_dir),$(CXXFILES:.cpp=.so))
EXTRA_DIST      += $(wildcard *.cpp)


# If extensions needing external libraries are to be compiled with this make
# file, each of them can be added according to this pattern:
# extra.so: extra.c
#	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<  [own compile and link flags]

jpg-load.so: jpg-load.c
	@if [ ! -z "$(LIBJPEG)" ]; then \
	echo $@;\
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(LIBJPEG);\
	fi

png-load.so: png-load.c $(GEGLHEADERS)
	@echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(PNG_CFLAGS) $(PNG_LIBS)

png-save.so: png-save.c $(GEGLHEADERS)
	@echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(PNG_CFLAGS) $(PNG_LIBS)

svg-load.so: svg-load.c
	@if [ ! -z "$(RSVG_LIBS)" ]; then \
	echo $@;\
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) \
                                     $(CAIRO_CFLAGS) $(CAIRO_LIBS) \
                                     $(RSVG_CFLAGS) $(RSVG_LIBS);\
	fi

pixbuf.so: pixbuf.c
	@if [ ! -z "$(RSVG_LIBS)" ]; then \
	echo $@;\
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(RSVG_CFLAGS) $(RSVG_LIBS);\
	fi

exr-load.so: exr-load.cpp
	@if [ ! -z "$(OPENEXR_LIBS)" ]; then \
	echo $@;\
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(OPENEXR_CFLAGS) $(OPENEXR_LIBS);\
	fi
