#!/bin/sh

if test -f config.log; then
   echo "you should rerun:"
   grep -o '\./configure .*$' config.log
else
   echo "you should now run the ./configure script"
fi
