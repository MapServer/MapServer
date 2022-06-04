#!/bin/bash

phpunit=`which phpunit`
ret=0

if  test -z "$phpunit" ; then
   echo "phpunit executable not found"
   exit 1
fi

if test -z $PHP_MAPSCRIPT_SO; then
   phpunit --debug .
   exit $?
else
   php -d "extension=$PHP_MAPSCRIPT_SO" $phpunit --debug .
   exit $?
fi
