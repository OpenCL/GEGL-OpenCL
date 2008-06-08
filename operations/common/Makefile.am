SUBDIRS = perlin
include $(top_srcdir)/operations/Makefile-operations.am

all-local: invert-simd.la

plugins += invert-simd.la

invert-simd.la: invert.c $(GEGLHEADERS)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -DSIMD_COMPILE -c -o $@.lo $<
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(AM_LDFLAGS) $(LDFLAGS) $(CFLAGS) $(LDADD) -o $@ -rpath $(ext_dir) $@.lo

