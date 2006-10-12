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

runinfo: Makefile.am
	if [ ! -z "$${CRUCIBLE_ID}" ]; then\
	  echo "<h2>Crucible</h2>" > runinfo;\
	  echo "<ul class='crucible'>" >> runinfo;\
	  for host in amd01 ita01 nfs08 nfs11 nfs12 ppc01;do echo "<li><a href='../$${host}/index.html'>$${host}</a></li>" >> runinfo; done;\
	  echo "</ul>" >> runinfo ;\
	  echo "<p>These pages document a GEGL build at a <a href='http://crucible.osdl.org/'>crucible</a> test machine.</p><p><a href='http://crucible.osdl.org/runs/$${CRUCIBLE_ID}/'>$${CRUCIBLE_ID}</a> @ $${CRUCIBLE_SUT} `date`</p>\
	  <pre>`cat /proc/cpuinfo`</pre>" >> runinfo;\
	fi

index.html: index-static.html                    \
	    $(top_srcdir)/AUTHORS                \
	    $(top_srcdir)/NEWS                   \
	    $(top_builddir)/bin/gegl		 \
	    Makefile.am				 \
	    runinfo
	echo -n "HTML: $@"
	cp $< $@ 
	$(top_srcdir)/tools/xml_insert.sh $@ AUTHORS $(top_srcdir)/AUTHORS
	[ -f runinfo ] && $(top_srcdir)/tools/xml_insert.sh $@ RUNINFO runinfo || true
	$(top_builddir)/bin/gegl -h > gegl-usage.txt 2>&1
	$(top_srcdir)/tools/xml_insert.sh $@ GEGL_USAGE gegl-usage.txt
	$(top_srcdir)/tools/xml_insert.sh $@ HELLO-WORLD hello-world.c encode
	rm gegl-usage.txt
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

WEBSITE_FILES=*.html *.css */*.html gallery/*.txt images/gegl.ico gallery/*.png

website:
	if [ "x"$$WEBSITE_DESTINATION == "x" ];then echo "WAR""NING: WEBSITE_DESTINATION not set";\
	else \
	    (mkdir -p $$WEBSITE_DESTINATION;\
	     cp --parents -rv  $(WEBSITE_FILES) $$WEBSITE_DESTINATION;);\
	fi
