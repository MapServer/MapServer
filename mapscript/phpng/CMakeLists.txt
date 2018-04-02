FIND_PACKAGE(PHP7 REQUIRED)
FIND_PACKAGE(SWIG 3.0.11 REQUIRED)
INCLUDE(${SWIG_USE_FILE})

include_directories(${PHP7_FOUND_INCLUDE_PATH})
include_directories(${PHP7_FOUND_INCLUDE_PATH}/main)
include_directories(${PHP7_FOUND_INCLUDE_PATH}/Zend)
include_directories(${PHP7_FOUND_INCLUDE_PATH}/TSRM)

include_directories(${PROJECT_SOURCE_DIR}/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/phpng)

if(WIN32)
include_directories(${PHP7_FOUND_INCLUDE_PATH}/win32)
set_property(SOURCE ../mapscript.i PROPERTY SWIG_FLAGS -DWIN32 -DZEND_WIN32 -DPHP_WIN32 -DZEND_DEBUG=0)
add_definitions(-DWIN32 -DZEND_WIN32 -DPHP_WIN32 -DZEND_DEBUG=0)
if(NOT CMAKE_CL_64)
set_property(SOURCE ../mapscript.i PROPERTY SWIG_FLAGS -D_USE_32BIT_TIME_T)
add_definitions(-D_USE_32BIT_TIME_T)
endif(NOT CMAKE_CL_64)
SWIG_LINK_LIBRARIES(phpng_mapscript ${PHP7_LIBRARY})
endif(WIN32)
SWIG_ADD_LIBRARY(phpng_mapscript TYPE MODULE LANGUAGE php7 SOURCES ../mapscript.i)

target_link_libraries(phpng_mapscript mapserver)

#set_target_properties(${SWIG_MODULE_phpng_mapscript_REAL_NAME} PROPERTIES PREFIX "phpng")
#set_target_properties(${SWIG_MODULE_phpng_mapscript_REAL_NAME} PROPERTIES OUTPUT_NAME phpng_mapscript)