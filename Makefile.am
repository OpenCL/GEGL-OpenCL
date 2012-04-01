ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

# The libs directory needs to be before anything which can depend on its
# libraries, otherwise any potentially rebuilt libs won't be picked up until
# the next run of make.
SUBDIRS=\
	libs \
	gegl \
	operations \
	bin \
	tools \
	examples \
	tests \
	po

if ENABLE_DOCS
SUBDIRS+= docs
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gegl-$(GEGL_API_VERSION).pc

built_dist_files = README

EXTRA_DIST = 			\
	$(built_dist_files)     \
	autogen.sh		\
	gegl-uninstalled.pc.in	\
	m4/introspection.m4	\
	gegl.pc.in

# DISTCHECK_CONFIGURE_FLAGS = --enable-introspection

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
