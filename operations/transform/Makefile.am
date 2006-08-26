# General minimalistic compile file for self contained single
# file babl_extensions

EXTRA_DIST=$(wildcard *.[ch])

CFILES  = $(wildcard $(srcdir)/*.c)
OFILES  = $(subst $(srcdir)/,,$(CFILES:.c=.o))
SOBJS   = affine.so

all-local: $(SOBJS)

CFLAGS  += @DEP_CFLAGS@ @BABL_CFLAGS@ \
           -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/gegl -I.
LDFLAGS += -shared 

LDADD    = @DEP_LIBS@ @BABL_LIBS@	

#win32_libs = -lgw32c -liberty -lole32 -luuid -lwsock32

%.o: %.c chant.h module.h matrix.h
	$(CC) $(CFLAGS) -c $< 

# If extensions needing external libraries are to be compiled with this make
# file, each of them can be added according to this pattern:
# extra.so: extra.c
#	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<  [own compile and link flags]

affine.so: $(OFILES)
	$(CC) $(LDFLAGS) $(LDADD) $(OFILES) -o affine.so
#############################################################################

clean-local:
	rm -f $(SOBJS) *.o *~

ext_dir = $(DESTDIR)$(prefix)/lib/gegl

install-exec-local: all-local
	$(INSTALL) -d $(ext_dir)
	$(INSTALL) $(SOBJS) $(ext_dir)

uninstall-local:
	rm -f $(ext_dir)/affine.so

check-local: all-local
