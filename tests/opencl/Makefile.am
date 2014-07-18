EXTRA_DIST=$(wildcard $(srcdir)/*.xml)

# Env vars to make binaries use builddir stuff
builddir_gegl_env = GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations

# Binary paths
gegl_bin = $(top_builddir)/bin/gegl$(EXEEXT)
img_cmp_bin = $(top_builddir)/tools/gegl-imgcmp$(EXEEXT)

# Commands run in a builddir env
builddir_gegl = $(builddir_gegl_env) $(gegl_bin)
builddir_img_cmp = $(builddir_gegl_env) $(img_cmp_bin)

# These strings must not be inside XML file names; we use $(subst ...)
testprefix = run-
testsuffix = .sh

# List of tests. The .sh suffix allows us to use $(wildcard $(srcdir)/*.xml) in
# EXTRA_DISTS
TESTS = \
  run-bilateral-filter.xml.sh    \
  run-box-blur.xml.sh            \
  run-brightness-contrast.xml.sh \
  run-color-temperature.xml.sh   \
  run-edge-sobel.xml.sh          \
  run-gaussian-blur.xml.sh       \
  run-invert-linear.xml.sh       \
  run-levels.xml.sh              \
  run-mono-mixer.xml.sh          \
  run-motion-blur.xml.sh         \
  run-noise-reduction.xml.sh     \
  run-oilify.xml.sh              \
  run-opacity.xml.sh             \
  run-pixelize.xml.sh            \
  run-snn-mean.xml.sh            \
  run-svg-src-over.xml.sh        \
  run-threshold.xml.sh           \
  run-value-invert.xml.sh        \
  run-vignette.xml.sh

# Create a separate executable script for each composition test to run
test_to_xml    = $(abs_srcdir)/$(subst $(testsuffix),,$(subst $(testprefix),,$(1)))
test_to_out    = $(abs_builddir)/output/$(basename $(notdir $(call test_to_xml,$(1)))).png
test_to_out_cl = $(abs_builddir)/output_cl/$(basename $(notdir $(call test_to_xml,$(1)))).png

$(testprefix)%.xml.sh: Makefile.am $(call test_to_xml,$@)
	@xml_file=$(call test_to_xml,$@)                                   ;\
	out_img=$(call test_to_out,$@)                                     ;\
	out_img_cl=$(call test_to_out_cl,$@)                               ;\
	echo "#!/bin/bash" > $@                                            ;\
	echo "mkdir -p $(abs_builddir)/output"    >> $@                    ;\
	echo "mkdir -p $(abs_builddir)/output_cl" >> $@                    ;\
	echo "GEGL_USE_OPENCL=no  $(builddir_gegl) $$xml_file -o $$out_img"    >> $@ ;\
	echo "GEGL_USE_OPENCL=yes $(builddir_gegl) $$xml_file -o $$out_img_cl" >> $@ ;\
	echo "$(builddir_img_cmp) $$out_img $$out_img_cl" >> $@            ;\
	chmod +x $@

clean-local:
	rm -f $(testprefix)*.xml$(testsuffix) output/* output_cl/*
