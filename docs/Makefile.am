.SILENT:
SUBDIRS= gallery
#DIST_SUBDIRS= graphics gallery

# The patterns used for html creation in the babl build system
# is very small, and should probably be documented in a tutorial.
#	
#
#
HTML_FILES = index-static.html 	\
	     index.html		\
	     operations.html	\
	     class-hierarchy.html

EXTRA_DIST= 			\
	ChangeLog		\
	gegl.css  		\
	todo.html		\
	index-static.html.in	\
	index.html

BUILT_EXTRA_DIST = index.html class-hierarchy.html operations.html

CLEANFILES = README $(HTML_FILES)
DISTCLEANFILES = index-static.html class-hierarchy.html operations.html
MAINTAINERCLEANFILES = $(BUILT_EXTRA_DIST)

all-local: $(HTML_FILES)

operations.html: $(top_builddir)/tools/operation_reference $(top_srcdir)/operations/*/*.c
	export GEGL_PATH=$(top_builddir)/operations;\
	$(top_builddir)/tools/operation_reference --ops-html > $@

class-hierarchy.html: $(top_builddir)/tools/introspect $(top_srcdir)/operations/*/*.c $(top_srcdir)/gegl/*.c
	export GEGL_PATH=$(top_builddir)/operations;\
	$(top_builddir)/tools/introspect > $@

index.html: index-static.html                    \
	    $(top_srcdir)/AUTHORS                \
	    $(top_srcdir)/NEWS                   \
	    Makefile.am	
	echo -n "HTML: $@"
	cp $< $@ 
	$(top_srcdir)/tools/xml_insert.sh $@ AUTHORS $(top_srcdir)/AUTHORS
	echo " [OK]"

distclean-local:
	if test $(srcdir) = .; then :; else 	\
		rm -f $(BUILT_EXTRA_DIST);	\
	fi

web: all
	echo -n " WEB: "
	ping -c 1 $(WEBSITE_HOST) >/dev/null && \
	   (\
	      scp *.txt *.html *.css\
	          "$(WEBSITE_HOST):$(WEBSITE_LOCATION)/" >/dev/null && \
		  echo -n "."; \
	      scp graphics/*.png graphics/*.html\
	          "$(WEBSITE_HOST):$(WEBSITE_LOCATION)/graphics/" >/dev/null\
		&&\
	          echo "[OK]"\
		||\
	          echo "FAIL   scp failed (does dirs exist?)."\
	   )\
	|| \
	   (\
	   echo "FAIL   $(WEBSITE_HOST) not reachable."\
	   )
