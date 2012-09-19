#!/bin/bash

ret=0

tests=( misc gdal renderers wxs )

for testcase in  "${tests[@]}" ; do
   cd msautotest/$testcase
   ./run_test.py 

   cd ../..
done

#tests failed, provide a summary
echo ""
echo "        Test run summary        "
echo "################################"
for testcase in "${tests[@]}"; do
   cd msautotest/$testcase
   echo ""
   if [ ! -d result ]; then
      #"result" directory does not exist, all tests passed
      cd ../..
      echo "$testcase: PASS"
      continue
   fi
   cd result
   #leftover .aux.xml files are valid for some gdal tests
   failedtests=`find . ! -name '*.aux.xml' -type f -printf "%f\n" `
   if [ -z "$failedtests" ]; then
      cd ../../..
      echo "$testcase: PASS"
      continue
   fi
   #we have some failing tests
   ret=1
   echo "!!!!! $testcase: FAIL !!!!!"
   echo "failing tests:"
   for failedtest in $failedtests; do
      echo $failedtest
      #for gml and xml files, print a diff
      if echo "$failedtest" | egrep -q "(xml|gml)$"; then
        diff -u "../expected/$failedtest" "$failedtest"
     fi
   done
   cd ../../..
done

# PHP tests
echo ""
echo "        PHP Tests        "
echo "#########################"
echo ""
cd msautotest/php
./run_test.py
if [ ! "$?" -eq 0 ]; then
    ret=1
fi

cd ../..

exit $ret
