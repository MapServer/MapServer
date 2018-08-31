find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})
find_package(PythonInterp)

# Python library/header finding doesn't seem to honor the python
# interpreter that was found beforehand, and defaults to the system
# python. We first try to find python.h and libpython.so ourselves
# from the hints given by distutils and sys
execute_process ( COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_inc; print(get_python_inc(True))" OUTPUT_VARIABLE PYTHON_INC OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process ( COMMAND ${PYTHON_EXECUTABLE} -c "import sys; print(sys.prefix)" OUTPUT_VARIABLE PYTHON_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)

find_path(PYTHON_INCLUDE_PATH Python.h
  HINTS ${PYTHON_INC}
)

find_library(PYTHON_LIBRARIES
  NAMES python${PYTHON_VERSION_STRING} python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} python
  HINTS ${PYTHON_PREFIX}/lib
)

#if that failed, fall back to the default cmake method of finding python dev files
if(NOT PYTHON_INCLUDE_PATH OR NOT PYTHON_LIBRARIES)
  find_package(PythonLibs)
endif()

include_directories(${PYTHON_INCLUDE_PATH})
include_directories(${PROJECT_SOURCE_DIR}/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/python)

if (${PYTHON_VERSION_MAJOR} GREATER 2)
    set_property(SOURCE ../mapscript.i PROPERTY SWIG_FLAGS "-py3")
endif ()

if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
    swig_add_library(pythonmapscript TYPE MODULE LANGUAGE python SOURCES ../mapscript.i)
else ()
    swig_add_module(pythonmapscript python ../mapscript.i)
endif ()

swig_link_libraries(pythonmapscript ${PYTHON_LIBRARIES} ${MAPSERVER_LIBMAPSERVER})

set_target_properties(${SWIG_MODULE_pythonmapscript_REAL_NAME} PROPERTIES PREFIX "")
set_target_properties(${SWIG_MODULE_pythonmapscript_REAL_NAME} PROPERTIES OUTPUT_NAME _mapscript)

if(MSVC)
	set_target_properties(${SWIG_MODULE_pythonmapscript_REAL_NAME} PROPERTIES LINK_FLAGS "/ignore:4244")
	set_target_properties(${SWIG_MODULE_pythonmapscript_REAL_NAME} PROPERTIES LINK_FLAGS "/ignore:4267")	
endif()

execute_process (COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(True))" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)

set(SETUP_PY_IN "${PROJECT_SOURCE_DIR}/mapscript/python/setup.py.in")
set(SETUP_PY_TEMP "${CMAKE_CURRENT_BINARY_DIR}/setup.py.temp")

configure_file(${SETUP_PY_IN} ${SETUP_PY_TEMP} @ONLY)
file(READ ${SETUP_PY_TEMP} SETUP_CONTENT)

# each target (e.g. Debug, Release etc.) will have a copy of setup.py

file(GENERATE OUTPUT $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/setup.py INPUT ${SETUP_PY_TEMP})

set(BUILD_TIMESTAMP "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")
set(TESTS_FOLDER ${PROJECT_SOURCE_DIR}/mapscript/python/tests/cases)

add_custom_target(pythonmapscript-wheel DEPENDS ${SWIG_MODULE_pythonmapscript_REAL_NAME} )

# copy all Python files to the output folder following build, and create a wheel

if (${CMAKE_BUILD_TYPE} STREQUAL "")
	set(OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})
else()
	set(OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif()

	
add_custom_command(
	WORKING_DIRECTORY ${OUTPUT_FOLDER}
    TARGET pythonmapscript-wheel POST_BUILD

    COMMAND ${PYTHON_EXECUTABLE} -m pip install -r ${PROJECT_SOURCE_DIR}/mapscript/python/requirements-dev.txt
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/mapscript/python/mapscript" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/mapscript.py" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/mapscript.py
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${SWIG_MODULE_pythonmapscript_REAL_NAME}>" "$<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/$<TARGET_FILE_NAME:${SWIG_MODULE_pythonmapscript_REAL_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/mapscript/python/README.rst" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/README.rst
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/mapscript/python/examples" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/examples
    COMMAND ${PYTHON_EXECUTABLE} setup.py bdist_wheel
    COMMAND ${PYTHON_EXECUTABLE} -m pip install --no-index --find-links=${CMAKE_CURRENT_BINARY_DIR}/dist mapscript
    COMMAND ${PYTHON_EXECUTABLE} -m pytest --ignore=${TESTS_FOLDER}/fonttest.py --ignore=${TESTS_FOLDER}/hashtest.py --ignore=${TESTS_FOLDER}/pgtest.py --ignore=${TESTS_FOLDER}/threadtest.py ${TESTS_FOLDER}/
)

set(mapscript_files $<TARGET_FILE:${SWIG_MODULE_pythonmapscript_REAL_NAME}> $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/mapscript.py)
install(FILES ${mapscript_files} DESTINATION ${PYTHON_SITE_PACKAGES})

message(STATUS "CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Python MapScript output directory: ${OUTPUT_FOLDER}")
message(STATUS "Python executable: ${PYTHON_EXECUTABLE}")
message(STATUS "Python site packages: ${PYTHON_SITE_PACKAGES}")
