find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})
find_package(PythonInterp)
find_python_module(setuptools REQUIRED)

# Python library/header finding doesn't seem to honor the python
# interpreter that was found beforehand, and defaults to the system
# python. We first try to find python.h and libpython.so ourselves
# from the hints given by distutils and sys

execute_process (COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_inc; print(get_python_inc(True))" OUTPUT_VARIABLE PYTHON_INC OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND ${PYTHON_EXECUTABLE} -c "import sys; print(sys.prefix)" OUTPUT_VARIABLE PYTHON_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(True))" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)

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

if (${PYTHON_VERSION_MAJOR} VERSION_GREATER 2)
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

set(SETUP_PY_IN "${PROJECT_SOURCE_DIR}/mapscript/python/setup.py.in")
set(SETUP_PY_TEMP "${CMAKE_CURRENT_BINARY_DIR}/setup.py.temp")

configure_file(${SETUP_PY_IN} ${SETUP_PY_TEMP} @ONLY)
file(READ ${SETUP_PY_TEMP} SETUP_CONTENT)

# each target (e.g. Debug, Release etc.) will have a copy of setup.py

file(GENERATE OUTPUT $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/setup.py INPUT ${SETUP_PY_TEMP})

# following not needed?
set(BUILD_TIMESTAMP "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")
set(TESTS_FOLDER ${PROJECT_SOURCE_DIR}/mapscript/python/tests/cases)

if(MSVC)
    set(OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
else()
    # for non-Windows builds there are no build type subfolders (e.g. Release, Debug etc.)
    set(OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(WIN32)
    set(PYTHON_SCRIPTS ${OUTPUT_FOLDER}/mapscriptvenv/Scripts)
    set(PYTHON_VENV_SCRIPTS ${OUTPUT_FOLDER}/mapscriptvenv/Scripts)
else()
    set(PYTHON_SCRIPTS ${OUTPUT_FOLDER}/mapscriptvenv/Scripts)
    set(PYTHON_VENV_SCRIPTS ${OUTPUT_FOLDER}/mapscriptvenv/bin)
endif()

add_custom_target(
    pythonmapscript-wheel
    DEPENDS mapscripttests.stamp
)

add_custom_command(
    DEPENDS ${SWIG_MODULE_pythonmapscript_REAL_NAME}
    OUTPUT mapscriptvenv.stamp
    WORKING_DIRECTORY ${OUTPUT_FOLDER}
    COMMENT "Creating a Python virtual environment"
    COMMAND ${PYTHON_EXECUTABLE} -m pip install virtualenv
    COMMAND ${PYTHON_EXECUTABLE} -m virtualenv mapscriptvenv
)

add_custom_command(
    DEPENDS mapscriptvenv.stamp
    OUTPUT mapscriptvenvpop.stamp
    WORKING_DIRECTORY ${OUTPUT_FOLDER}
    COMMENT "Installing the required packages into the virtual environment"
    COMMAND ${PYTHON_VENV_SCRIPTS}/python -m pip install pip --upgrade --force-reinstall
    COMMAND ${PYTHON_VENV_SCRIPTS}/pip install -r ${PROJECT_SOURCE_DIR}/mapscript/python/requirements-dev.txt
)

add_custom_command(
    DEPENDS mapscriptvenvpop.stamp
    OUTPUT mapscriptlinting.stamp
    WORKING_DIRECTORY ${OUTPUT_FOLDER}
    COMMENT "Linting test code files with flake8"
    COMMAND ${PYTHON_VENV_SCRIPTS}/flake8 $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/tests --max-line-length=120
)

add_custom_command(
    DEPENDS mapscriptlinting.stamp
    OUTPUT mapscriptwheel.stamp
    WORKING_DIRECTORY ${OUTPUT_FOLDER}
    # now check the Python test code with flake8
    COMMENT "Linting test code files"
    COMMAND ${PYTHON_VENV_SCRIPTS}/flake8 $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/tests --max-line-length=120
    COMMENT "Building the mapscript Python wheel"
    COMMAND ${PYTHON_VENV_SCRIPTS}/python setup.py bdist_wheel
)

add_custom_command(
    DEPENDS mapscriptwheel.stamp
    OUTPUT mapscripttests.stamp
    COMMENT "Installing the Python wheel and testing it in the virtual environment"
    COMMAND ${PYTHON_VENV_SCRIPTS}/pip install --no-index --find-links=${OUTPUT_FOLDER}/dist mapscript
    COMMAND ${PYTHON_VENV_SCRIPTS}/pytest --pyargs mapscript.tests
)

add_custom_command(
  TARGET _pythonmapscript
  WORKING_DIRECTORY ${OUTPUT_FOLDER}
  POST_BUILD
  COMMENT "Copying files required to build Mapscript"
  COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/mapscript/python/mapscript/__init__.py" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/__init__.py
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/mapscript.py" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/mapscript.py
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${SWIG_MODULE_pythonmapscript_REAL_NAME}>" "$<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/$<TARGET_FILE_NAME:${SWIG_MODULE_pythonmapscript_REAL_NAME}>"
  COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/mapscript/python/README.rst" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/README.rst
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/mapscript/python/tests/cases" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/tests
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/tests" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/tests/data
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/mapscript/python/examples" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/examples
)

install(  
  CODE "
  
    SET(ENV{PYTHONPATH} \${PYTHON_SITE_PACKAGES}:\$ENV{PYTHONPATH})
      
    if(DEFINED ENV{DESTDIR})
      SET(PYTHON_ROOT \"--root=\$ENV{DESTDIR}\")
    endif()
  
    if(DEFINED CMAKE_INSTALL_PREFIX)
      SET(PYTHON_PREFIX \"--prefix=\${CMAKE_INSTALL_PREFIX}\")
    endif()

    execute_process(
      COMMAND ${PYTHON_EXECUTABLE} setup.py install \${PYTHON_ROOT} \${PYTHON_PREFIX}
      WORKING_DIRECTORY ${OUTPUT_FOLDER}
    )
  "
)

message(STATUS "CMake Version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Python MapScript output directory: ${OUTPUT_FOLDER}")
message(STATUS "Python Executable: ${PYTHON_EXECUTABLE}")
message(STATUS "Python site packages: ${PYTHON_SITE_PACKAGES}")