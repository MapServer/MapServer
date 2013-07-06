#!/bin/sh
lcov --directory . --capture --output-file mapserver.info 
#gcc filter_info.c -o filter_info
#./filter_info < gdal.info > gdal_filtered.info 
#genhtml -o ./coverage_html --num-spaces 2 gdal_filtered.info
cp /usr/bin/genhtml .
# to remove date, so that only real differences show up
patch -p0 < genhtml.patch
./genhtml -o ./mapserver_coverage_html --num-spaces 2 mapserver.info  
lftp -c "set ftp:list-options -a; open $MY_FTP_REPO; get mapserver_coverage_html.txt"
find mapserver_coverage_html -type f -exec md5sum {} \; > local_mapserver_coverage_html.txt
PUT_AND_RM=$(python find_updated_files.py)
cp local_mapserver_coverage_html.txt mapserver_coverage_html.txt
echo $PUT_AND_RM
#lftp -c "set ftp:list-options -a; open $MY_FTP_REPO; lcd ./mapserver_coverage_html; cd ./mapserver_coverage_html; mirror --reverse --delete --use-cache --verbose --allow-chown --allow-suid --no-umask --parallel=2 --exclude-glob .svn"
lftp -c "set ftp:list-options -a; open $MY_FTP_REPO; $PUT_AND_RM; put mapserver_coverage_html.txt;"
#mkdir /tmp/ftpperso
#curlftpfs $MY_FTP_REPO /tmp/ftpperso/
#rsync -av mapserver_coverage_html /tmp/ftpperso
