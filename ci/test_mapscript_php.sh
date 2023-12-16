# upgrade to recent PHPUnit

cd msautotest
cd php && curl -LO https://phar.phpunit.de/phpunit-10.phar
cd ..

# check for phpunit & xdebug
php msautotest/php/phpunit-10.phar --version
php -v

make phpng-build
