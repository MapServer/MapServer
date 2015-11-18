#!/bin/bash

commitdiff=$1

if test -z $commitdiff; then
   echo "usage: $0 startcommit..endcommit"
   exit
fi

git --no-pager  log --no-merges  --pretty=format:'%s (%an) : `%h <https://github.com/mapserver/mapserver/commit/%H>`__' $commitdiff | gsed  's!#\([0-9]\+\)! `#\1 <https://github.com/mapserver/mapserver/issues/\1>`__ !g'

