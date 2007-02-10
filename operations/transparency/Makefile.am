EXTRA_DIST=$(wildcard *.c)

CFILES  = $(wildcard $(srcdir)/*.c)
SOBJS   = $(subst $(srcdir)/,,$(CFILES:.c=.so))
INSTALLED_ITEMS = $(subst $(srcdir),$(ext_dir),$(CFILES:.c=.so))
GEGLHEADERS = $(wildcard $(top_srcdir)/gegl/*.h)\
	      $(wildcard $(top_srcdir)/gegl/buffer/*.h)

all-local: $(SOBJS)

CFLAGS  += @DEP_CFLAGS@ @BABL_CFLAGS@ \
           -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/gegl -I$(top_srcdir)/gegl/buffer -I. -fPIC
LDFLAGS += -shared 

LDADD    = @DEP_LIBS@ @BABL_LIBS@

#win32_libs = -lgw32c -liberty -lole32 -luuid -lwsock32

%.so: %.c $(GEGLHEADERS)
	@echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD)

clean-local:
	rm -f $(SOBJS) *~

ext_dir = $(DESTDIR)$(prefix)/lib/gegl-@GEGL_API_VERSION@

install-exec-local: all-local
	$(INSTALL) -d $(ext_dir)
	$(INSTALL) $(SOBJS) $(ext_dir)

uninstall-local:
	rm -f $(INSTALLED_ITEMS)

check-local: all-local
