#!/bin/sh

# Set by TESTS_ENVIRONMENT in Makefile.am
reference_dir=$REFERENCE_DIR

total_tests=`ls -1 $reference_dir/*.buf | wc -l`
successful_tests=`cat buffer-tests-report | grep -c identical`

echo $successful_tests of $total_tests tests succesful.

if [ $total_tests -eq $successful_tests ]
then
exit 0;
else
echo Look in the file \"buffer-tests-report\" for detailed failure information.
exit 1;
fi
