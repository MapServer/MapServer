#!/bin/bash

ret=0

for testcase in misc gdal renderers wxs; do
   logfile="/tmp/logfile-$testcase.log"
   exec > >(tee $logfile)
   exec 2>&1

   cd msautotest/$testcase
   ./run_test.py

   grep -q "^0 tests failed" $logfile
   RETVAL=$?
   [ $RETVAL -ne 0 ] && ret=1
   cd ../..
done

exit $ret
