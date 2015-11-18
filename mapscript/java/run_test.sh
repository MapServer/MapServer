#!/bin/sh -ex

JAVA=${JAVA_HOME}/bin/java
JAVAC=${JAVA_HOME}/bin/javac

${JAVAC} -classpath ./:examples/:${JAVA_MAPSCRIPT_SO}/mapscript.jar -d examples/ examples/RFC24.java examples/ConnPool.java examples/DrawMap.java examples/DumpShp.java examples/MakePoint.java examples/QueryByAttribute.java examples/ShapeInfo.java examples/WxSTest.java examples/Metadata.java examples/RunTimeBuiltWMSClient.java examples/WxSTestNoThread.java
${JAVAC} -encoding utf8 -classpath ./:examples/:${JAVA_MAPSCRIPT_SO}/mapscript.jar -d examples/ examples/QueryByAttributeUnicode.java 
${JAVA} -classpath ./:examples/:${JAVA_MAPSCRIPT_SO}/mapscript.jar -Djava.library.path=${JAVA_MAPSCRIPT_SO} DumpShp ../../tests/point.shp
${JAVA} -classpath ./:examples/:${JAVA_MAPSCRIPT_SO}/mapscript.jar -Djava.library.path=${JAVA_MAPSCRIPT_SO} ShapeInfo ../../tests/point.shp ../../tests/point.dbf
${JAVA} -classpath ./:examples/:${JAVA_MAPSCRIPT_SO}/mapscript.jar -Djava.library.path=${JAVA_MAPSCRIPT_SO} DrawMap ../../tests/test.map ./map.png
${JAVA} -classpath ./:examples/:${JAVA_MAPSCRIPT_SO}/mapscript.jar -Djava.library.path=${JAVA_MAPSCRIPT_SO} ConnPool
${JAVA} -classpath ./:examples/:${JAVA_MAPSCRIPT_SO}/mapscript.jar -Djava.library.path=${JAVA_MAPSCRIPT_SO} QueryByAttribute ../../tests/test.map
${JAVA} -classpath ./:examples/:${JAVA_MAPSCRIPT_SO}/mapscript.jar -Djava.library.path=${JAVA_MAPSCRIPT_SO} WxSTest ../../tests/test.map
${JAVA} -classpath ./:examples/:${JAVA_MAPSCRIPT_SO}/mapscript.jar -Djava.library.path=${JAVA_MAPSCRIPT_SO} WxSTestNoThread ../../tests/test.map
${JAVA} -classpath ./:examples/:${JAVA_MAPSCRIPT_SO}/mapscript.jar -Djava.library.path=${JAVA_MAPSCRIPT_SO} RFC24 ../../tests/test.map
LC_ALL=en_US.UTF-8 ${JAVA} -classpath ./:examples/:${JAVA_MAPSCRIPT_SO}/mapscript.jar -Djava.library.path=${JAVA_MAPSCRIPT_SO} QueryByAttributeUnicode data/subset-umlauts.map

${JAVAC} -cp ${JAVA_MAPSCRIPT_SO}/mapscript.jar -d tests/threadtest/ tests/threadtest/*.java
#-------------------------------------------------------------------------
#  You can use your own map! Copy the following command in your shell
#      and change the file to the map file (the last argument)
#-------------------------------------------------------------------------
${JAVA} -Djava.library.path=${JAVA_MAPSCRIPT_SO} -classpath tests/threadtest/:${JAVA_MAPSCRIPT_SO}/mapscript.jar MapTest -t 50 -i 5 ../../tests/test.map
