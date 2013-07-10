#!/bin/sh
#lcov --directory . --capture --output-file mapserver.info 

#cp /usr/bin/genhtml .
# to remove date, so that only real differences show up
#patch -p0 < genhtml.patch

#genhtml -o ./mapserver_coverage_html --num-spaces 2 mapserver.info  

#echo "$SSH_DEPLOY_KEY" > ~/.ssh/id_rsa-msautotest-coverage-result

# build the chunks of the private key together

echo $id_rsa_00 | base64 --decode | head -n1

echo -n $id_rsa_{00..30} >> ~/.ssh/id_rsa-msautotest-coverage-results_base64
base64 --decode --ignore-garbage ~/.ssh/id_rsa-msautotest-coverage-results_base64 > ~/.ssh/id_rsa-msautotest-coverage-results
chmod 600 ~/.ssh/id_rsa-msautotest-coverage-results

# debug
head -n1 ~/.ssh/id_rsa-msautotest-coverage-results
tail -n1 ~/.ssh/id_rsa-msautotest-coverage-results

exit 0

echo "Host foo.github.com" >> ~/.ssh/config
echo "    StrictHostKeyChecking no" >> ~/.ssh/config
echo "    Hostname github.com" >> ~/.ssh/config
echo "    IdentityFile ~/.ssh/id_rsa-msautotest-coverage-results" >> ~/.ssh/config

ssh -v -T git@foo.github.com

# public key
#echo "ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEA6l8sBIw8JCry/PSv86JTDjjE8G67+D9f04cn6ac/C4H7E0tiBTXcTggQi2V+VfkTvJQ7gFffwAsV7E3zECN8cd25u9f2LIkr5dvdOovOvWy8mjaONQaaWzUk8einbBhJOgbZNYihtGO09qTZOzoBJrizzoKtsAxqmfEq8cQdINtkLb1mcp+DCATS7qPC0R6mYuigMvf2B9rsRHtJL4nvqJ23yTh7ZamAVGFRc2XgHU/0lTA3WWExHxZo06Ve81jafbv4ePw307RCsLo4nlBnWFHkzcmGwcDk1ZGtuI+KT0Q0kSFLPoz40z6X7j7xKD60HV8TjuC2ezwlgGzrIkcuDQ== https://github.com/rouault/msautotest-coverage-results" >>  ~/.ssh/id_rsa-msautotest-coverage-results.pub

mkdir msautotest-coverage-results
cd msautotest-coverage-results
git init
git config user.email "mapserverbot@mapserver.bot"
git config user.name "MapServer coveragebot"
git config push.default matching
git remote add origin git@foo.github.com:rouault/msautotest-coverage-results.git
git fetch
git pull origin master
#rm -rf *
#cp -r ../mapserver_coverage_html .
echo "Results of coverage of MapServer msautotest" > README.md
echo "See http://rawgithub.com/rouault/msautotest-coverage-results/master/home/travis/build/rouault/mapserver/index.html" >> README.md
git add -A
git commit -m "update with results of commit $TRAVIS_COMMIT"
git push origin master


# download last updated filelist
#lftp -c "set ftp:list-options -a; open $MY_FTP_REPO; get mapserver_coverage_html.txt"

#find mapserver_coverage_html -type f -exec md5sum {} \; > local_mapserver_coverage_html.txt
# compute incremental update
#PUT_AND_RM=$(python find_updated_files.py)
#echo $PUT_AND_RM

#cp local_mapserver_coverage_html.txt mapserver_coverage_html.txt

# upload files
#lftp -c "set ftp:list-options -a; open $MY_FTP_REPO; $PUT_AND_RM; put mapserver_coverage_html.txt;"

