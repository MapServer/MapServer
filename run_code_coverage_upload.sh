#!/bin/sh
lcov --directory . --capture --output-file mapserver.info 

cp /usr/bin/genhtml .
# to remove date, so that only real differences show up
patch -p0 < genhtml.patch

./genhtml -o ./mapserver_coverage_html --num-spaces 2 mapserver.info  

# download last updated filelist
lftp -c "set ftp:list-options -a; open $MY_FTP_REPO; get mapserver_coverage_html.txt"

find mapserver_coverage_html -type f -exec md5sum {} \; > local_mapserver_coverage_html.txt
# compute incremental update
PUT_AND_RM=$(python find_updated_files.py)
echo $PUT_AND_RM

cp local_mapserver_coverage_html.txt mapserver_coverage_html.txt

# upload files
lftp -c "set ftp:list-options -a; open $MY_FTP_REPO; $PUT_AND_RM; put mapserver_coverage_html.txt;"
