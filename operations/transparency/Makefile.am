# General minimalistic compile file for self contained single
# file babl_extensions

#win32_libs = -lgw32c -liberty -lole32 -luuid -lwsock32

CFILES  = $(wildcard *.c)
SOBJS   = $(CFILES:.c=.so)
all: $(SOBJS)

CFLAGS  += -Wall -I../..

%.so: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD)

LDFLAGS += -shared 

LDADD    = @DEP_LIBS@ @BABL_LIBS@	
CFLAGS  += @DEP_CFLAGS@ @BABL_CFLAGS@ -I$(top_srcdir) -I$(top_srcdir)/gegl -I.

clean:
	rm -f $(SOBJS) *~

ext_dir      = /usr/local/lib/gegl

install: all
	$(INSTALL) -d $(ext_dir)
	$(INSTALL) $(SOBJS) $(ext_dir)

check: all
