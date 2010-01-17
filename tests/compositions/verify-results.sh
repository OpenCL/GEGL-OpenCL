#!/bin/sh
total_tests=`ls -1 reference | wc -l`
successful_tests=`cat tests-report | grep identical | wc -l`

if [ $total_tests -eq $successful_tests ]
then
echo $successful_tests of $total_tests tests succesful.
exit 0;
else
cat tests-report
echo -------------------------
echo $successful_tests of $total_tests tests succesful.
exit 1;
fi
