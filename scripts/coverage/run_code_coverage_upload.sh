#!/bin/bash
lcov --directory . --capture --output-file mapserver.info 

cp /usr/bin/genhtml .
# to remove date and occurences number, so that only real differences show up
patch -p0 < genhtml.patch

./genhtml -o ./mapserver_coverage_html --num-spaces 2 mapserver.info  

# build the chunks of the private key together

# This is a bashism !
echo -n $id_rsa_{00..30} >> ~/.ssh/id_rsa-upstream-msautotest-coverage-results_base64
base64 --decode --ignore-garbage ~/.ssh/id_rsa-upstream-msautotest-coverage-results_base64 > ~/.ssh/id_rsa-upstream-msautotest-coverage-results
chmod 600 ~/.ssh/id_rsa-upstream-msautotest-coverage-results

echo "Host foo.github.com" >> ~/.ssh/config
echo "    StrictHostKeyChecking no" >> ~/.ssh/config
echo "    Hostname github.com" >> ~/.ssh/config
echo "    IdentityFile ~/.ssh/id_rsa-upstream-msautotest-coverage-results" >> ~/.ssh/config

mkdir msautotest-coverage-results
cd msautotest-coverage-results
git init
git config user.email "mapserverbot@mapserver.bot"
git config user.name "MapServer coveragebot"
git config push.default matching
git remote add origin git@foo.github.com:mapserver/coverage.git
git fetch
git pull origin master
rm -rf *
cp -r ../mapserver_coverage_html .
echo "Results of coverage of MapServer msautotest" > README.md
echo "See http://rawgithub.com/mapserver/coverage/master/mapserver_coverage_html/home/travis/build/mapserver/mapserver/index.html" >> README.md
git add -A
git commit -m "update with results of commit https://github.com/mapserver/mapserver/commit/$TRAVIS_COMMIT"
git push origin master
