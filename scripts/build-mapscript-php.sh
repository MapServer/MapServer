#!/bin/bash
set -eu  # Exit on error and treat unset variables as errors

cd "$WORK_DIR"

# Ensure the Python version is provided
if [ -z "${PHP_VERSION:-}" ]; then
    echo "Error: PHP version not specified. Make sure to pass it as an environment variable."
    exit 1
fi

export DEBIAN_FRONTEND=noninteractive

apt-get update -y

# general dependencies
apt-get install -y sudo software-properties-common wget curl git python3 python3-pip
sudo ln -s /usr/bin/python3 /usr/bin/python


# for pre-installed software see
# https://github.com/actions/runner-images/blob/main/images/ubuntu/Ubuntu2404-Readme.md
# both swig and php are included, so removed these
sudo apt-get -qq remove "php*-cli" "php*-dev"
sudo apt-get -qq remove "swig*"

# https://github.com/oerdnj/deb.sury.org/issues/56
add-apt-repository ppa:ondrej/php -y
LC_ALL=C.UTF-8 add-apt-repository ppa:ondrej/php -y

sudo apt-get -qq update
sudo apt-get -qq install php"${PHP_VERSION}"-cli php"${PHP_VERSION}"-dev php"${PHP_VERSION}"-mbstring php"${PHP_VERSION}"-xml php"${PHP_VERSION}"-pcov php"${PHP_VERSION}"-xdebug

# install build dependencies
ci/ubuntu/setup.sh

# build the project
export CC="ccache gcc"
export CXX="ccache g++"

make cmakebuild_mapscript_php

cd "$WORK_DIR"

# get PHPUnit
cd msautotest
echo "PHP version"
php -v
PHPVersionMinor=$(php --version | head -n 1 | cut -d " " -f 2 | cut -c 1,3)
if [ ${PHPVersionMinor} -ge 83 ]; then
    cd php && curl -LO https://phar.phpunit.de/phpunit-12.phar
    echo "PHPUnit version"
    php phpunit-12.phar --version
else
    cd php && curl -LO https://phar.phpunit.de/phpunit-11.phar
    echo "PHPUnit version"
    php phpunit-11.phar --version
fi

echo "PHP includes"

php-config --includes

cd "$WORK_DIR"

make php-testcase

echo "Done!"