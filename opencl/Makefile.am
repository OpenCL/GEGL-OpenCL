EXTRA_DIST = $(wildcard $(srcdir)/*.h) $(wildcard $(srcdir)/*.py)

SOURCES = $(wildcard $(srcdir)/*.cl)

BUILT_SOURCES = $(subst .cl,.cl.h,$(wildcard $(srcdir)/*.cl))

%.cl.h: %.cl $(srcdir)/cltostring.py
	$(PYTHON) $(srcdir)/cltostring.py $<