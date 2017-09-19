#!/bin/bash

msa_commit=`git log -n1 | grep  "msautotest=" | sed 's/msautotest=//'`
branch=master
repo=git://github.com/mapserver/msautotest.git
if [ -n "$msa_commit" ]; then
  repo=`echo "$msa_commit" | grep -o '^[^@]*'`
  branch=`echo "$msa_commit" | grep -o '[^@]*$'`
fi
echo "git clone $repo msautotest"
git clone $repo msautotest
cd msautotest
echo "git checkout $branch"
git checkout $branch
