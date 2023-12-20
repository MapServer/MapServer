# upgrade to recent PHPUnit

echo "Running PHP tests"

cd msautotest
cd php && curl -LO https://phar.phpunit.de/phpunit-10.phar
cd ../../

# check for phpunit & xdebug
php msautotest/php/phpunit-10.phar --version
php -v

# -DWITH_PHPNG=1 is currently always true so no need to rebuild here
# make phpng-build


if ! [ "${WITH_ASAN:-}" = "true" ]; then
    # skip PHP tests when running WITH_ASAN
    make php-testcase
fi