find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

find_package(JNI)
find_package(Java)
if(NOT JNI_INCLUDE_DIRS OR NOT Java_JAVAC_EXECUTABLE OR NOT Java_JAR_EXECUTABLE)
   message(SEND_ERROR "Could not find required Java components. Try setting the JAVA_HOME environment variable (required on e.g. Ubuntu)")
endif(NOT JNI_INCLUDE_DIRS OR NOT Java_JAVAC_EXECUTABLE OR NOT Java_JAR_EXECUTABLE)

include_directories(${JNI_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/java)
include_directories(${PROJECT_BINARY_DIR}/src/mapscript/)
set (CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/edu/umn/gis/mapscript")
set(CMAKE_SWIG_FLAGS -package edu.umn.gis.mapscript)

# hide warnings
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
endif ()


if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
    swig_add_library(javamapscript TYPE MODULE LANGUAGE java SOURCES ../mapscript.i)
else ()
    swig_add_module(javamapscript java ../mapscript.i)
endif ()

swig_link_libraries(javamapscript ${MAPSERVER_LIBMAPSERVER})

add_custom_command(TARGET javamapscript
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      POST_BUILD
                      COMMAND ${Java_JAVAC_EXECUTABLE} edu/umn/gis/mapscript/*.java
                      COMMAND ${Java_JAR_EXECUTABLE} cf mapscript.jar edu/umn/gis/mapscript/*.class
                      COMMENT "Compiling java source files, creating mapscript.jar"
                      )

set(mapscript_files $<TARGET_FILE:${SWIG_MODULE_javamapscript_REAL_NAME}>)
install(FILES ${mapscript_files} DESTINATION ${CMAKE_INSTALL_LIBDIR})
