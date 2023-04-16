#!/bin/bash

if test -z $PHP_MAPSCRIPT_SO; then
   php phpunit-10.phar .
   exit $?
else
   php -d "extension=$PHP_MAPSCRIPT_SO" phpunit-10.phar .
   exit $?
fi
