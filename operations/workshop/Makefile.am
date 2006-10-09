SUBDIRS = generated

EXTRA_DIST      = $(wildcard *.c) $(wildcard perlin/*.[ch])
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

clean-local:
	rm -f $(SOBJS) *~

ext_dir = $(DESTDIR)$(prefix)/lib/gegl

install-exec-local: all-local
	$(INSTALL) -d $(ext_dir)
	-$(INSTALL) $(SOBJS) $(ext_dir)

uninstall-local:
	rm -f $(INSTALLED_ITEMS)

check-local: all-local


%.so: %.c $(GEGLHEADERS)
	@echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD)

gst.so: gst.c $(GEGLHEADERS)
	-$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) `pkg-config gstreamer-0.10 --libs --cflags`

gtk-display.so: gtk-display.c $(GEGLHEADERS)
	@[ ! -z "$(GTK_LIBS)" ] &&\
	(echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(GTK_CFLAGS) $(GTK_LIBS))\
	||true


