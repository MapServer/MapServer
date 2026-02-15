find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

if(WIN32)
  FIND_PACKAGE(SWIG 3.0.11 REQUIRED)
  if(WITH_THREAD_SAFETY)
    FIND_PACKAGE(SWIG 4.0.0 REQUIRED)
  endif(WITH_THREAD_SAFETY)
endif(WIN32)

FIND_PACKAGE(PHP REQUIRED)
INCLUDE(UseSWIG)

if(NOT PHP_EXTENSION_DIR)
    execute_process(COMMAND php-config --extension-dir 
                    OUTPUT_VARIABLE PHP_EXTENSION_DIR 
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(NOT PHP_EXTENSION_DIR)
    message(WARNING "PHP_EXTENSION_DIR is empty after php-config probe. Defaulting to lib/php/extensions")
    set(PHP_EXTENSION_DIR "lib/php/extensions")
endif()

message(STATUS "PHP Extension Directory: ${PHP_EXTENSION_DIR}")


# Try to find standard PHP include directories if not already set
if(NOT PHP_INCLUDE_PATH AND PHP_INCLUDE_DIRS)
    set(PHP_INCLUDE_PATH ${PHP_INCLUDE_DIRS})
endif()

if(NOT PHP_INCLUDE_PATH)
    # Explicitly find php-config executable
    find_program(PHP_CONFIG_EXECUTABLE NAMES php-config php-config8 php-config7)
    
    if(PHP_CONFIG_EXECUTABLE)
        execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --includes 
                        OUTPUT_VARIABLE PHP_O_INCLUDE_PATH 
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        RESULT_VARIABLE PHP_CONFIG_RESULT)
        
        if(PHP_CONFIG_RESULT EQUAL 0 AND PHP_O_INCLUDE_PATH)
            string(REPLACE "-I" "" PHP_INCLUDE_PATH "${PHP_O_INCLUDE_PATH}") 
            string(REPLACE " " ";" PHP_INCLUDE_PATH "${PHP_INCLUDE_PATH}")
        endif()
    endif()
endif()

if(NOT PHP_INCLUDE_PATH)
    message(WARNING "PHP_INCLUDE_PATH is empty. Attempting to guess based on standard locations.")
    # Common locations on Linux/macOS
    find_path(PHP_H_LOCATION php.h PATHS 
        /usr/include/php
        /usr/local/include/php
        /opt/homebrew/include/php
        /usr/include/php/main
        /usr/local/include/php/main
        ${CMAKE_PREFIX_PATH}/include/php
        ${CMAKE_PREFIX_PATH}/include/php/main
        ENV PHP_INCLUDE_DIR
    )
    if(PHP_H_LOCATION)
        set(PHP_INCLUDE_PATH ${PHP_H_LOCATION})
        # Add subdirectories often needed (TSRM, Zend, main)
        list(APPEND PHP_INCLUDE_PATH "${PHP_H_LOCATION}/main" "${PHP_H_LOCATION}/TSRM" "${PHP_H_LOCATION}/Zend")
    endif()
endif()

# Sanity check: Ensure php.h exists in at least one of the include paths
find_path(PHP_H_CHECK "php.h" PATHS ${PHP_INCLUDE_PATH} NO_DEFAULT_PATH)
if(NOT PHP_H_CHECK)
    message(FATAL_ERROR "Could not find php.h in include paths: '${PHP_INCLUDE_PATH}'. Please install php-dev or set PHP_INCLUDE_PATH manually.")
endif()

message(STATUS "Final PHP Include Path: ${PHP_INCLUDE_PATH}")

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

include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/phpng)
include_directories(${PROJECT_BINARY_DIR}/src/mapscript/)
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
  target_link_libraries(php_mapscriptng PRIVATE ${PHP_LIBRARY})
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
  install(TARGETS php_mapscriptng LIBRARY DESTINATION ${PHP_EXTENSION_DIR})
  # check if we must install the required file containing MapServer constants and functions
  if(${SWIG_VERSION} LESS_EQUAL 4.0.2)
     install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mapscript.php DESTINATION ${PHP_EXTENSION_DIR} COMPONENT dev)
  endif()
endif()
