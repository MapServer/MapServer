#!/bin/sh
lcov --directory . --capture --output-file mapserver.info 
#gcc filter_info.c -o filter_info
#./filter_info < gdal.info > gdal_filtered.info 
#genhtml -o ./coverage_html --num-spaces 2 gdal_filtered.info
genhtml -o ./mapserver_coverage_html --num-spaces 2 mapserver.info  
mkdir /tmp/ftpperso
curlftpfs $MY_FTP_REPO /tmp/ftpperso/
rsync -av mapserver_coverage_html /tmp/ftpperso
