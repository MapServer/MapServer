#!/bin/bash

ret=0

tests=( query misc gdal wxs renderers )
command_exists () {
   type "$1" &> /dev/null ;
}

DIFF=diff
COMPARE=
if command_exists colordiff ; then
   DIFF="colordiff"
fi
if command_exists compare ; then
   COMPARE="compare"
fi

#leftover .aux.xml files are valid for some gdal tests
rm -f msautotest/*/result/*.aux.xml

for testcase in "${tests[@]}"; do
   cd msautotest/$testcase
   if [ ! -d result ]; then
      #"result" directory does not exist, all tests passed
      cd ../..
      continue
   fi
   cd result
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
      if echo "$failedtest" | egrep -q "(png|gif|tif)$"; then
        if echo "$failedtest" | egrep -v -q "\\.diff\\.png$"; then
           if [ -n "$COMPARE" ]; then
              $COMPARE ../expected/$failedtest $failedtest -compose Src $failedtest.diff.png
           fi
        fi
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
