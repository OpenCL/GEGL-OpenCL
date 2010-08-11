#!/bin/sh

# Set by TESTS_ENVIRONMENT in Makefile.am
abs_top_srcdir=$ABS_TOP_SRCDIR
abs_top_builddir=$ABS_TOP_BUILDDIR

if [ ! -f $abs_top_builddir/tools/exp_combine ]; then
  echo "Skipping test-exp-combine due to lack of exp_combine executable"
  failure=0
else
  $abs_top_builddir/tools/exp_combine                              \
          $abs_top_builddir/tests/simple/test-exp-combine.hdr      \
          $abs_top_srcdir/tests/compositions/data/parliament_0.jpg \
          $abs_top_srcdir/tests/compositions/data/parliament_1.jpg \
          $abs_top_srcdir/tests/compositions/data/parliament_2.jpg \
  && $abs_top_builddir/tools/img_cmp                               \
          $abs_top_builddir/tests/simple/test-exp-combine.hdr      \
          $abs_top_srcdir/tests/compositions/data/parliament.hdr >/dev/null
  failure=$?
  [ ! $failure -eq 0 ] && echo "imp_cmp failed (we need to fix the test so it passes on many architectures), see parliament-diff.png"
  rm -f $abs_top_builddir/tests/simple/test-exp-combine.hdr
fi

exit $failure
