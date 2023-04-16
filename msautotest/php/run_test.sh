#!/bin/bash

if test -z $PHP_MAPSCRIPT_SO; then
   php phpunit-10.phar --debug .
   exit $?
else
   php -d "extension=$PHP_MAPSCRIPT_SO" phpunit-9.5.phar --debug .
   exit $?
fi
