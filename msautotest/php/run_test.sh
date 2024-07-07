#!/bin/bash

if test -z $PHP_MAPSCRIPT_SO; then
   php phpunit-11.phar .
   exit $?
else
   php -d "extension=$PHP_MAPSCRIPT_SO" phpunit-11.phar .
   exit $?
fi
