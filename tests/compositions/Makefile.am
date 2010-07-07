
# The setup is as follows:
#  * Each test comes with one <name>.xml GEGL XML and one reference
#    output file in 'reference' called <name>.<ext>, where <ext> is a
#    supported output file type.
#  * The test is run by putting the XML into a file in 'output', named
#    <name>.<ext> and then compared with the file in 'reference'. If
#    these files are equal, the test is a 'pass'.


SUBDIRS=data

EXTRA_DIST=$(wildcard *.xml) $(wildcard reference/*) verify-results.sh

# Env vars to make binaries use builddir stuff
builddir_gegl_env = GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations

# Binary paths
gegl_bin = $(top_builddir)/bin/gegl$(EXEEXT)
img_cmp_bin = $(top_builddir)/tools/img_cmp$(EXEEXT)

# Commands run in a builddir env
builddir_gegl = $(builddir_gegl_env) $(gegl_bin)
builddir_img_cmp = $(builddir_gegl_env) $(img_cmp_bin)


# For simplicity, use the same time stamp for output of all
# compositions. Use the same output file as in the reference dir
images.stamp: $(wildcard $(srcdir)/*.xml) \
              $(wildcard $(top_builddir)/operations/.libs/*$(SHREXT)) \
              $(wildcard $(top_builddir)/operations/*/.libs/*$(SHREXT)) \
              $(wildcard $(top_builddir)/operations/*/*/.libs/*$(SHREXT)) \
	      $(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la \
	      $(gegl_bin)
	@echo "--[doing XML composition test renderings]--"
	@mkdir -p ./output
	@for XML in $(srcdir)/*.xml; do \
	    echo $$XML; \
	    export BASE=`echo $$XML | sed s?$(srcdir)/?? | sed s/\.xml//`;\
	    $(builddir_gegl) $$XML -o output/`basename $(srcdir)/reference/$$BASE.*`; \
	done
	@touch images.stamp

# Read by verify-results.sh
TESTS_ENVIRONMENT = VERIFY_RESULTS_REFERENCE=$(srcdir)/reference

TESTS = verify-results.sh
verify-results.sh: tests-report

tests-report: images.stamp \
              $(top_srcdir)/tools/img_cmp \
              $(srcdir)/reference/*
	@echo "--[Verifying test renderings against references]--"
	@echo "" > tests-report ;
	@for XML in $(srcdir)/*.xml; do\
	    echo $$XML; \
	    export BASE=`echo $$XML | sed s?$(srcdir)/?? | sed s/\.xml//`;\
	    $(builddir_img_cmp) $(srcdir)/reference/$$BASE.* output/$$BASE.* >> tests-report; true ;\
	   done;

clean-local:
	rm -f images.stamp output/* *.txt tests-report
