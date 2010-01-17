SUBDIRS=data

EXTRA_DIST=$(wildcard *.xml) $(wildcard reference/*.png) verify-results.sh

$(builddir)/output/%.png: %.xml
	@echo $@ > /tmp/a ;\
	GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations \
	$(top_builddir)/bin/gegl$(EXEEXT) $< -o $@ 

images.stamp: $(wildcard $(builddir)/*.xml) \
              $(wildcard $(top_builddir)/operations/*/*$(SHREXT)) \
	      $(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la \
	      $(top_builddir)/bin/gegl$(EXEEXT)
	@echo "--[updating test renders]--"; \
	mkdir output || true > /dev/null;
	test -e $(builddir)/reference || ln -s $(srcdir)/reference $(builddir)/reference || true > /dev/null;
	for XML in $(srcdir)/*.xml; do \
	    export BASE=`echo $$XML | sed s?$(srcdir)/?? | sed s/\.xml//`;\
            PNG=$$BASE".png";\
	    echo $${XML}; \
	   $(MAKE) $(builddir)"/output/$$PNG" >/dev/null; \
	done
	@touch images.stamp


TESTS = verify-results.sh
verify-results.sh: tests-report

tests-report: images.stamp $(top_srcdir)/tools/img_cmp
	@echo "" > tests-report ;
	@for XML in $(srcdir)/*.xml; do\
	    export BASE=`echo $$XML | sed s?$(srcdir)/?? | sed s/\.xml//`;\
	    PNG=$$BASE".png";\
            GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations \
	    $(top_builddir)/tools/img_cmp $(srcdir)/reference/$$PNG $(builddir)/output/$$PNG >> tests-report; true ;\
	   done;

clean-local:
	rm -f images.stamp *.png output/*.png *.txt tests-report
