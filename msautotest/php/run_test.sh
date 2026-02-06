#!/bin/bash

cd "$WORK_DIR"

# get PHPUnit
cd msautotest
echo "PHP version"
php -v
PHPVersionMinor=$(php --version | head -n 1 | cut -d " " -f 2 | cut -c 1,3)
if [ ${PHPVersionMinor} -ge 84 ]; then
    PHPUnitVersion=13
else
    PHPUnitVersion=10
fi
cd php && curl -LO https://phar.phpunit.de/phpunit-$PHPUnitVersion.phar
echo "PHPUnit version"
php phpunit-$PHPUnitVersion.phar --version

if test -z $PHP_MAPSCRIPT_SO; then
   php phpunit-$PHPUnitVersion.phar .
   exit $?
else
   php -d "extension=$PHP_MAPSCRIPT_SO" phpunit-$PHPUnitVersion.phar .
   exit $?
fi
