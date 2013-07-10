#!/bin/sh
lcov --directory . --capture --output-file mapserver.info 

#cp /usr/bin/genhtml .
# to remove date, so that only real differences show up
#patch -p0 < genhtml.patch

genhtml -o ./mapserver_coverage_html --num-spaces 2 mapserver.info  

#echo "$SSH_DEPLOY_KEY" > ~/.ssh/id_rsa-msautotest-coverage-result

echo -n $id_rsa_{00..30} >> ~/.ssh/id_rsa-msautotest-coverage-results_base64
base64 --decode --ignore-garbage ~/.ssh/id_rsa-msautotest-coverage-results_base64 > ~/.ssh/id_rsa-msautotest-coverage-results
chmod 600 ~/.ssh/id_rsa-msautotest-coverage-results

echo "Host foo.github.com" >> ~/.ssh/config
echo "    StrictHostKeyChecking no" >> ~/.ssh/config
echo "    Hostname github.com" >> ~/.ssh/config
echo "    IdentityFile ~/.ssh/id_rsa-msautotest-coverage-results" >> ~/.ssh/config

mkdir msautotest-coverage-results
git init
git config user.email "mapserverbot@mapserver.bot"
git config user.name "MapServer coveragebot"
git config push.default matching
git remote add origin git@foo.github.com:rouault/msautotest-coverage-results.git
git fetch
git pull origin master
rm -rf *
cp -r ../mapserver_coverage_html .
echo "Results of coverage of MapServer msautotest" > README.md
echo "See http://rawgithub.com/rouault/msautotest-coverage-results/master/home/travis/build/rouault/mapserver/index.html" >> README.md
git add -A
git commit -m "update"
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

