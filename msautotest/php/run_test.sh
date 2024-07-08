#!/bin/bash

PHPVersionMinor=$(php --version | head -n 1 | cut -d " " -f 2 | cut -c 1,3)
if [ ${PHPVersionMinor} -gt 81 ]; then
    PHPUnitVersion=11
else
    PHPUnitVersion=10
fi

if test -z $PHP_MAPSCRIPT_SO; then
   php phpunit-$PHPUnitVersion.phar .
   exit $?
else
   php -d "extension=$PHP_MAPSCRIPT_SO" phpunit-$PHPUnitVersion.phar .
   exit $?
fi
