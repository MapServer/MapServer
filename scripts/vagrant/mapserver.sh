NUMTHREADS=2
if [[ -f /sys/devices/system/cpu/online ]]; then
	# Calculates 1.5 times physical threads
	NUMTHREADS=$(( ( $(cut -f 2 -d '-' /sys/devices/system/cpu/online) + 1 ) * 15 / 10  ))
fi
#NUMTHREADS=1 # disable MP
export NUMTHREADS


git clone https://github.com/mapserver/mapserver.git mapserver
cd mapserver
git submodule init
git checkout branch-6-4
git submodule update
cd msautotest
python -m SimpleHTTPServer &> /dev/null &
cd ..
mkdir build
touch maplexer.l
touch mapparser.y
flex --nounistd -Pmsyy -i -omaplexer.c maplexer.l
yacc -d -omapparser.c mapparser.y
cd build
cmake   -G "Unix Makefiles"  \
        -DWITH_GD=1 -DWITH_CLIENT_WMS=1 \
			  -DWITH_CLIENT_WFS=1 -DWITH_KML=1 -DWITH_SOS=1 -DWITH_PHP=1 \
			  -DWITH_PYTHON=1 -DWITH_JAVA=0 -DWITH_THREAD_SAFETY=1 -DWITH_FRIBIDI=0 -DWITH_FCGI=0 -DWITH_EXEMPI=1 \
			  -DCMAKE_BUILD_TYPE=Release -DWITH_RSVG=1 -DWITH_CURL=1 \
        ..

make -j $NUMTHREADS
sudo make install

