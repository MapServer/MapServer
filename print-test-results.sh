#!/bin/bash

ret=0

tests=( query misc gdal wxs renderers )
command_exists () {
   type "$1" &> /dev/null ;
}

DIFF=diff
if command_exists colordiff ; then
   DIFF="colordiff"
fi

for testcase in "${tests[@]}"; do
   cd msautotest/$testcase
   if [ ! -d result ]; then
      #"result" directory does not exist, all tests passed
      cd ../..
      continue
   fi
   cd result
   rm -f *.aux.xml
   #leftover .aux.xml files are valid for some gdal tests
   failedtests=`find . -type f -printf "%f\n" `
   if [ -z "$failedtests" ]; then
      cd ../../..
      continue
   fi
   #we have some failing tests
   ret=1
   for failedtest in $failedtests; do
      echo ""
      echo "######################################"
      echo "# $testcase => $failedtest"
      echo "######################################"
      #for txt, gml and xml files, print a diff
      if echo "$failedtest" | egrep -q "(txt|xml|gml)$"; then
        $DIFF -u "../expected/$failedtest" "$failedtest"
     fi
   done
   cd ../../..
done

if test "$ret" -eq "0"; then
   echo ""
   echo "######################################"
   echo "#       All tests passed             #"
   echo "######################################"
   echo ""
fi

exit $ret
