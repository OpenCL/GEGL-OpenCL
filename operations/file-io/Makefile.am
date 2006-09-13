EXTRA_DIST      = $(wildcard *.c)
CFILES          = $(wildcard $(srcdir)/*.c)
SOBJS           = $(subst $(srcdir)/,,$(CFILES:.c=.so))
INSTALLED_ITEMS = $(subst $(srcdir),$(ext_dir),$(CFILES:.c=.so))
GEGLHEADERS     = $(wildcard $(top_srcdir)/gegl/*.h)\
	          $(wildcard $(top_srcdir)/gegl/buffer/*.h)

all-local: $(SOBJS)

CFLAGS  += @DEP_CFLAGS@ @BABL_CFLAGS@ \
           -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/gegl -I. -fPIC
LDFLAGS += -shared 

LDADD    = @DEP_LIBS@ @BABL_LIBS@

#win32_libs = -lgw32c -liberty -lole32 -luuid -lwsock32

%.so: %.c $(GEGLHEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD)

# If extensions needing external libraries are to be compiled with this make
# file, each of them can be added according to this pattern:
# extra.so: extra.c
#	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<  [own compile and link flags]

jpg-load.so: jpg-load.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(LIBJPEG)

png-load.so: png-load.c $(GEGLHEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(PNG_CFLAGS) $(PNG_LIBS)
png-save.so: png-save.c $(GEGLHEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(PNG_CFLAGS) $(PNG_LIBS)

svg-load.so: svg-load.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) \
                                     $(CAIRO_CFLAGS) $(CAIRO_LIBS) \
                                     $(RSVG_CFLAGS) $(RSVG_LIBS)
#############################################################################

clean-local:
	rm -f $(SOBJS) *~

ext_dir = $(DESTDIR)$(prefix)/lib/gegl

install-exec-local: all-local
	$(INSTALL) -d $(ext_dir)
	-$(INSTALL) $(SOBJS) $(ext_dir)

uninstall-local:
	rm -f $(INSTALLED_ITEMS)

check-local: all-local
