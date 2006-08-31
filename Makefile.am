AUTOMAKE_OPTIONS = dist-bzip2

SUBDIRS = gegl operations tools docs

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gegl.pc

built_dist_files = README

EXTRA_DIST = 			\
	$(built_dist_files)     \
	gegl-uninstalled.pc.in	\
	gegl.pc.in

DISTCLEANFILES = \
	$(built_dist_files)

if HAVE_W3M
dist-check-w3m:

README: all docs/index.html
	$(W3M) -cols 72 -dump docs/index.html > $@
else
dist-check-w3m:
	@echo "*** w3m must be available in order to make dist"
	@false
endif

dist-hook: dist-check-w3m
