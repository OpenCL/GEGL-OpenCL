# General minimalistic compile file for self contained single
# file babl_extensions

#win32_libs = -lgw32c -liberty -lole32 -luuid -lwsock32

EXTRA_DIST=*.[ch]
CFILES  = $(wildcard *.c)
OFILES  = $(CFILES:.c=.o)
SOBJS   = affine.so
all: $(SOBJS)

CFLAGS  += -Wall -I../..

affine.so: $(OFILES)
	$(CC) $(LDFLAGS) $(LDADD) $(OFILES) -o affine.so

%.o: %.c chant.h module.h matrix.h
	$(CC) $(CFLAGS) -c $< 

LDFLAGS += -shared 

LDADD    = @DEP_LIBS@ @BABL_LIBS@	
CFLAGS  += @DEP_CFLAGS@ @BABL_CFLAGS@ -I$(top_srcdir) -I$(top_srcdir)/gegl -I.

clean:
	rm -f $(SOBJS) $(OFILES) *~

ext_dir = $(DESTDIR)$(prefix)/lib/gegl

install: all
	$(INSTALL) -d $(ext_dir)
	$(INSTALL) $(SOBJS) $(ext_dir)

check: all
