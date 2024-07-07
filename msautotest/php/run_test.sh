#!/bin/bash

if test -z $PHP_MAPSCRIPT_SO; then
   php phpunit-$PHPUnitVersion.phar .
   exit $?
else
   php -d "extension=$PHP_MAPSCRIPT_SO" phpunit-$PHPUnitVersion.phar .
   exit $?
fi
