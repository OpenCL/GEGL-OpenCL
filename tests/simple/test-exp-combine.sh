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
          $abs_top_srcdir/tests/compositions/data/parliament.hdr   \
          $abs_top_builddir/tests/simple/test-exp-combine.hdr
  failure=$?
  if [ $failure -eq 0 ]; then
    rm -f $abs_top_builddir/tests/simple/test-exp-combine.hdr
  fi
fi

exit $failure
