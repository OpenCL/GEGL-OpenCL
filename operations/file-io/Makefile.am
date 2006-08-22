# General minimalistic compile file for self contained single
# file babl_extensions

#win32_libs = -lgw32c -liberty -lole32 -luuid -lwsock32

EXTRA_DIST=*.c
CFILES  = $(wildcard *.c)
SOBJS   = $(CFILES:.c=.so)
all: $(SOBJS)

CFLAGS  += -Wall -I../..

%.so: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD)

# if extension needing external libraries are to be # compiled with this make
# file, each of them can be added according to the this pattern:
# extra.so: extra.c
#	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<  [own compile and link flags]

png-load.so: png-load.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(PNG_CFLAGS) $(PNG_LIBS)
png-save.so: png-save.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(PNG_CFLAGS) $(PNG_LIBS)

jpg-load.so: jpg-load.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(LIBJPEG)
#############################################################################
LDFLAGS += -shared 

LDADD    = @DEP_LIBS@ @BABL_LIBS@	
CFLAGS  += @DEP_CFLAGS@ @BABL_CFLAGS@ -I$(top_srcdir) -I$(top_srcdir)/gegl -I.

clean:
	rm -f $(SOBJS) *~

ext_dir = $(DESTDIR)$(prefix)/lib/gegl

install: all
	$(INSTALL) -d $(ext_dir)
	$(INSTALL) $(SOBJS) $(ext_dir)

check: all
