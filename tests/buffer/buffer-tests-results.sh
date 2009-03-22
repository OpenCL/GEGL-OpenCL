#!/bin/sh
total_tests=`ls -1 reference | wc -l`
successful_tests=`cat buffer-tests-report | grep identical | wc -l`

echo $successful_tests of $total_tests tests succesful.

if [ $total_tests -eq $successful_tests ]
then
exit 0;
else
echo Look in the file \"buffer-tests-report\" for detailed failure information.
exit -1;
fi
