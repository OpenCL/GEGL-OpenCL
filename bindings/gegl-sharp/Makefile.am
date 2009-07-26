SUBDIRS = gegl sample #doc

EXTRA_DIST = 			\
	ChangeLog.pre-0-1

### ChangeLog generation

CHANGELOG_START = 5e2cc2146486ce0c1ebc0cbf68770dafafcb123f
GIT_ROOT = "$(top_srcdir)/../../"

ChangeLog: $(srcdir)/ChangeLog
$(srcdir)/ChangeLog:
	@echo Creating $@
	@if test -d $(GIT_ROOT)/.git; then \
	  (GIT_DIR=$(GIT_ROOT)/.git $(GIT_ROOT)/missing --run \
	   git log $(CHANGELOG_START)^.. --stat -- bindings/gegl-sharp) | fmt --split-only > $@.tmp \
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
