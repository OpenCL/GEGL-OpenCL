AUTOMAKE_OPTIONS = dist-bzip2

SUBDIRS=\
	gegl \
	operations \
	bin \
	tools \
	examples \
	tests

if ENABLE_DOCS
SUBDIRS+= docs
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gegl.pc

built_dist_files = README

EXTRA_DIST = 			\
	ChangeLog.pre-0-1	\
	$(built_dist_files)     \
	autogen.sh		\
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

website: all
	$(MAKE) -C docs website


### ChangeLog generation

CHANGELOG_START = 5e2cc2146486ce0c1ebc0cbf68770dafafcb123f

ChangeLog: $(srcdir)/ChangeLog
$(srcdir)/ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run \
	   git log $(CHANGELOG_START)^.. --stat) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> $@); \
	fi
.PHONY: $(srcdir)/ChangeLog
