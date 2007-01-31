EXTRA_DIST      = $(wildcard *.[ch])
CFILES          = $(wildcard $(srcdir)/*.c)
OFILES          = $(subst $(srcdir)/,,$(CFILES:.c=.o))
GEGLHEADERS     = $(wildcard $(top_srcdir)/gegl/*.h)\
                  $(wildcard $(top_srcdir)/gegl/buffer/*.h)

SOBJS           = affine.so

all-local: $(SOBJS)

CFLAGS  += @DEP_CFLAGS@ @BABL_CFLAGS@ \
           -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/gegl -I$(top_srcdir)/gegl/buffer -I. -fPIC
LDFLAGS += -shared 

LDADD    = @DEP_LIBS@ @BABL_LIBS@

#win32_libs = -lgw32c -liberty -lole32 -luuid -lwsock32

%.o: %.c chant.h module.h matrix.h $(GEGLHEADERS)
	@echo $@;$(CC) $(CFLAGS) -c $< 

affine.so: $(OFILES)
	@echo $@;$(CC) $(LDFLAGS) $(LDADD) $(OFILES) -o affine.so

clean-local:
	rm -f $(SOBJS) *.o *~

ext_dir = $(DESTDIR)$(prefix)/lib/gegl

install-exec-local: all-local
	$(INSTALL) -d $(ext_dir)
	$(INSTALL) $(SOBJS) $(ext_dir)

uninstall-local:
	rm -f $(ext_dir)/affine.so

check-local: all-local
