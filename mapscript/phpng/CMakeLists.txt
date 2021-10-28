find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})
if(WIN32 AND ((${CMAKE_VERSION} EQUAL "3.12.0") OR (${CMAKE_VERSION} EQUAL "3.12.1") OR (${CMAKE_VERSION} EQUAL "3.12.2") OR (${CMAKE_VERSION} EQUAL "3.12.3")))
    message(FATAL_ERROR "Building with CMAKE ${CMAKE_VERSION} will not work. Please downgrade to 3.11.x or update to a higher version than 3.12.3")
endif()

if(WIN32)
  FIND_PACKAGE(SWIG 3.0.11 REQUIRED)
  if(WITH_THREAD_SAFETY)
    FIND_PACKAGE(SWIG 4.0.0 REQUIRED)
  endif(WITH_THREAD_SAFETY)
endif(WIN32)

FIND_PACKAGE(PHP REQUIRED)
INCLUDE(UseSWIG)

if(WIN32)
  add_definitions(/DWIN32 /DZEND_WIN32 /DPHP_WIN32 /DZEND_DEBUG=0)
  if(NOT CMAKE_CL_64)
    add_definitions(-D_USE_32BIT_TIME_T)
  endif(NOT CMAKE_CL_64)
  if(WITH_THREAD_SAFETY)
    add_definitions(-DZTS=1)
  endif(WITH_THREAD_SAFETY)
endif(WIN32)

include_directories(${PHP_INCLUDE_PATH})
message(STATUS "PHP Include Path: ${PHP_INCLUDE_PATH}")

include_directories(${PROJECT_SOURCE_DIR}/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/phpng)
include_directories(${CMAKE_CURRENT_BINARY_DIR}) # make sure mapscriptPHP7_wrap.c is found

if(WIN32)
  include_directories(${PHP_INCLUDE_PATH}/win32)
endif(WIN32)

IF(PHP_VERSION LESS 5)
  MESSAGE(FATAL_ERROR "PHP version is not 5 or later")
ENDIF(PHP_VERSION LESS 5)

IF(PHP_VERSION LESS 7)
  if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
      SWIG_ADD_LIBRARY(php_mapscriptng TYPE MODULE LANGUAGE php SOURCES ../mapscript.i)
  else ()
      swig_add_module(php_mapscriptng php ../mapscript.i)
  endif ()
ELSE()
  if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
      SWIG_ADD_LIBRARY(php_mapscriptng TYPE MODULE LANGUAGE php7 SOURCES ../mapscript.i)
  else ()
      swig_add_module(php_mapscriptng php7 ../mapscript.i)
  endif ()
ENDIF(PHP_VERSION LESS 7)

if(WIN32)
  target_compile_options(php_mapscriptng PRIVATE /DWIN32 /DZEND_WIN32 /DPHP_WIN32 /DZEND_DEBUG=0)
  if(WITH_THREAD_SAFETY)
    target_compile_options(php_mapscriptng PRIVATE /DZTS=1)
  endif(WITH_THREAD_SAFETY)
  SWIG_LINK_LIBRARIES(php_mapscriptng ${PHP_LIBRARY})
endif(WIN32)

IF(APPLE)
  if(XCODE)
     SET(CMAKE_C_LINK_FLAGS 
        "${CMAKE_C_LINK_FLAGS} -undefined dynamic_lookup")
     SET(CMAKE_CXX_LINK_FLAGS 
        "${CMAKE_CXX_LINK_FLAGS} -undefined dynamic_lookup")
  else(XCODE)
     SET(CMAKE_SHARED_MODULE_CREATE_C_FLAGS 
        "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -undefined dynamic_lookup")
     SET(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS 
        "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -undefined dynamic_lookup")
  endif(XCODE)
ENDIF(APPLE)

# hide "defined but not used" warnings
target_compile_options(php_mapscriptng PRIVATE -Wno-unused-label)

target_link_libraries(php_mapscriptng mapserver)
set_target_properties(${SWIG_MODULE_mapscript_REAL_NAME} PROPERTIES PREFIX "")

if(NOT WIN32)
  install(TARGETS php_mapscriptng DESTINATION ${PHP_EXTENSION_DIR})
  # install the required file containing MapServer constants and functions
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mapscript.php DESTINATION ${PHP_EXTENSION_DIR} COMPONENT dev)
endif()
