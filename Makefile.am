# Process this file with automake to create Makefile.in.

SUBDIRS = gegl operations docs tools

EXTRA_DIST = 			\
	gegl-uninstalled.pc.in	\
	gegl.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gegl.pc

built_dist_files = README
if HAVE_W3M
dist-check-w3m:

README: docs/index.html
	$(W3M) -cols 72 -dump $< > $@
else
dist-check-w3m:
	@echo "*** w3m must be available in order to make dist"
	@false
endif

