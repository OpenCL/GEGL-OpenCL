
# The setup is as follows:
#  * Each test comes with one <name>.xml GEGL XML and one reference
#    output file in 'reference' called <name>.<ext>, where <ext> is a
#    supported output file type.
#  * The test is run by processing the GEGL XML into an output file in
#    'output', named <name>.<ext>, and then compared with the file in
#    'reference'. If these files are equal, the test is a 'pass'.


SUBDIRS=data

EXTRA_DIST=$(wildcard $(srcdir)/*.xml) $(wildcard $(srcdir)/reference/*)

# Env vars to make binaries use builddir stuff
builddir_gegl_env = GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations

# Binary paths
gegl_bin = $(top_builddir)/bin/gegl$(EXEEXT)
img_cmp_bin = $(top_builddir)/tools/img_cmp$(EXEEXT)

# Commands run in a builddir env
builddir_gegl = $(builddir_gegl_env) $(gegl_bin)
builddir_img_cmp = $(builddir_gegl_env) $(img_cmp_bin)

# These strings must not be inside XML file names; we use $(subst ...)
testprefix = run-
testsuffix = .sh

# List of tests. The .sh suffix allows us to use $(wildcard $(srcdir)/*.xml) in
# EXTRA_DISTS
TESTS = \
  run-affine-with-negative.xml.sh      \
  run-clones.xml.sh                    \
  run-composite-transform.xml.sh       \
  run-edge-laplace-broken.xml.sh       \
  run-edge-sobel.xml.sh                \
  run-fattal02.xml.sh                  \
  run-gamma.xml.sh                     \
  run-hdr-color.xml.sh                 \
  run-mantiuk06.xml.sh                 \
  run-pixelize.xml.sh                  \
  run-reflect.xml.sh                   \
  run-reflect2.xml.sh                  \
  run-reinhard05.xml.sh                \
  run-rgb-params.xml.sh                \
  run-rgbe-load.xml.sh                 \
  run-rgbe-save.xml.sh                 \
  run-rotate.xml.sh                    \
  run-simple-scale.xml.sh              \
  run-softglow.xml.sh                  \
  run-transform.xml.sh                 \
  run-rectangles.xml.sh

  #run-matting-global.xml.sh           
  #run-matting-global-big.xml.sh     

# Conditional tests
if HAVE_JASPER
TESTS += run-jp2-load.xml.sh
endif
if HAVE_UMFPACK
TESTS += run-matting-levin.xml.sh
endif

# Create a separate executable script for each composition test to run
test_to_xml = $(abs_srcdir)/$(subst $(testsuffix),,$(subst $(testprefix),,$(1)))
test_to_ref = $(wildcard $(abs_srcdir)/reference/$(basename $(notdir $(call test_to_xml,$(1)))).*)
test_to_out = $(abs_builddir)/output/$(notdir $(call test_to_ref,$(1)))
$(testprefix)%.xml.sh: Makefile.am $(call test_to_xml,$@) $(call test_to_ref,$@)
	@xml_file=$(call test_to_xml,$@)                                   ;\
	ref_img=$(call test_to_ref,$@)                                     ;\
	out_img=$(call test_to_out,$@)                                     ;\
	echo "#!/bin/bash" > $@                                            ;\
	echo "mkdir -p $(abs_builddir)/output" >> $@                       ;\
	echo "if [ -a $$out_img ]; then rm $$out_img ; fi" >> $@           ;\
	echo "$(builddir_gegl) $$xml_file -o $$out_img" >> $@              ;\
	echo "$(builddir_img_cmp) $$ref_img $$out_img" >> $@  ;\
	chmod +x $@

clean-local:
	rm -f $(testprefix)*.xml$(testsuffix) output/*
