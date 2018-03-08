FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

FIND_PACKAGE(JNI)
FIND_PACKAGE(Java)
if(NOT JNI_INCLUDE_DIRS OR NOT Java_JAVAC_EXECUTABLE OR NOT Java_JAR_EXECUTABLE)
   message(SEND_ERROR "Could not find required Java componenents. Try setting the JAVA_HOME environment variable (required on e.g. Ubuntu)")
endif(NOT JNI_INCLUDE_DIRS OR NOT Java_JAVAC_EXECUTABLE OR NOT Java_JAR_EXECUTABLE)

INCLUDE_DIRECTORIES(${JNI_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/java)
SET (CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/edu/umn/gis/mapscript")
SET(CMAKE_SWIG_FLAGS -package edu.umn.gis.mapscript)
SWIG_ADD_MODULE(javamapscript java ../mapscript.i)

SWIG_LINK_LIBRARIES(javamapscript ${MAPSERVER_LIBMAPSERVER})

ADD_CUSTOM_COMMAND(TARGET javamapscript
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      POST_BUILD
                      COMMAND ${Java_JAVAC_EXECUTABLE} edu/umn/gis/mapscript/*.java
                      COMMAND ${Java_JAR_EXECUTABLE} cf mapscript.jar edu/umn/gis/mapscript/*.class
                      COMMENT "Compiling java source files, creating mapscript.jar"
                      )

get_target_property(LOC_MAPSCRIPT_LIB ${SWIG_MODULE_javamapscript_REAL_NAME} LOCATION)
install(FILES ${LOC_MAPSCRIPT_LIB} DESTINATION ${CMAKE_INSTALL_LIBDIR})

