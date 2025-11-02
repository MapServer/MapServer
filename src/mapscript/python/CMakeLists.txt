find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

find_package (Python REQUIRED COMPONENTS Interpreter Development)

include_directories(${Python_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/python)
include_directories(${PROJECT_BINARY_DIR}/src/mapscript/)

set(SwigFile ${PROJECT_SOURCE_DIR}/src/mapscript/mapscript.i)

# add and comments annotations to py file output - requires >= SWIG 4.1
if (WITH_PYMAPSCRIPT_ANNOTATIONS AND ${Python_VERSION_MAJOR} GREATER_EQUAL 3)
  set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS -doxygen)
  if (${Python_VERSION_MINOR} GREATER_EQUAL 6)
     set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS -features python:annotations=c)
  else ()
     set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS -features python:annotations=c,python:annotations:novar)
  endif ()
endif ()

if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
    swig_add_library(pythonmapscript TYPE MODULE LANGUAGE python SOURCES ${SwigFile})
else ()
    swig_add_module(pythonmapscript python ${SwigFile})
endif ()

swig_link_libraries(pythonmapscript ${Python_LIBRARIES} ${MAPSERVER_LIBMAPSERVER})

set_target_properties(${SWIG_MODULE_pythonmapscript_REAL_NAME} PROPERTIES PREFIX "")
set_target_properties(${SWIG_MODULE_pythonmapscript_REAL_NAME} PROPERTIES OUTPUT_NAME _mapscript)

set(SETUP_PY_IN "${PROJECT_SOURCE_DIR}/src/mapscript/python/setup.py.in")
set(SETUP_PY_TEMP "${CMAKE_CURRENT_BINARY_DIR}/setup.py.temp")

configure_file(${SETUP_PY_IN} ${SETUP_PY_TEMP} @ONLY)
file(READ ${SETUP_PY_TEMP} SETUP_CONTENT)

# each target (e.g. Debug, Release etc.) will have a copy of setup.py

file(GENERATE OUTPUT $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/setup.py INPUT ${SETUP_PY_TEMP})

IF (NOT DEFINED MAPSCRIPT_WORKING_DIR)
    if(MSVC)
        set(MAPSCRIPT_WORKING_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    else()
        # for non-Windows builds there are no build type subfolders (e.g. Release, Debug etc.)
        set(MAPSCRIPT_WORKING_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()
ENDIF (NOT DEFINED MAPSCRIPT_WORKING_DIR)

if(WIN32)
    # Windows venv binaries are in a different location than Linux
    set(Python_VENV_SCRIPTS ${MAPSCRIPT_WORKING_DIR}/mapscriptvenv/Scripts)
else()
    set(Python_VENV_SCRIPTS ${MAPSCRIPT_WORKING_DIR}/mapscriptvenv/bin)
endif()

add_custom_target(
    pythonmapscript-wheel
    DEPENDS ${MAPSCRIPT_WORKING_DIR}/mapscripttests.stamp
)

add_custom_command(
    DEPENDS ${SWIG_MODULE_pythonmapscript_REAL_NAME}
    OUTPUT ${MAPSCRIPT_WORKING_DIR}/mapscriptvenv.stamp
    WORKING_DIRECTORY ${MAPSCRIPT_WORKING_DIR}
    COMMAND ${Python_EXECUTABLE} -m venv mapscriptvenv
    COMMAND ${CMAKE_COMMAND} -E env PIP_NO_PYTHON_VERSION_WARNING=1
            ${Python_VENV_SCRIPTS}/pip install --no-warn-script-location  -r ${PROJECT_SOURCE_DIR}/src/mapscript/python/requirements-dev.txt > requires.log
    COMMAND ${CMAKE_COMMAND} -E touch ${MAPSCRIPT_WORKING_DIR}/mapscriptvenv.stamp
    COMMENT "Creating a Python virtual environment and installing the required packages"
)

add_custom_command(
    DEPENDS ${MAPSCRIPT_WORKING_DIR}/mapscriptvenv.stamp
    OUTPUT ${MAPSCRIPT_WORKING_DIR}/mapscriptwheel.stamp
    WORKING_DIRECTORY ${MAPSCRIPT_WORKING_DIR}
    COMMAND ${Python_VENV_SCRIPTS}/python -m build --wheel > wheel_build.log
    COMMAND ${CMAKE_COMMAND} -E touch ${MAPSCRIPT_WORKING_DIR}/mapscriptwheel.stamp
    COMMENT "Building the mapscript Python wheel"
)

add_custom_command(
    WORKING_DIRECTORY ${Python_VENV_SCRIPTS} # make sure scripts aren't run when from the same folder as mapscript.py
    DEPENDS ${MAPSCRIPT_WORKING_DIR}/mapscriptwheel.stamp
    OUTPUT ${MAPSCRIPT_WORKING_DIR}/mapscripttests.stamp
    COMMAND ${Python_VENV_SCRIPTS}/pip install --no-index --find-links=${MAPSCRIPT_WORKING_DIR}/dist mapscript
    # ERROR: file or package not found: mapscript.tests (missing __init__.py?) is caused by
    # ImportError: DLL load failed while importing _mapscript: The specified module could not be found.
    COMMAND ${Python_VENV_SCRIPTS}/python -m pytest --pyargs mapscript.tests
    COMMAND ${Python_VENV_SCRIPTS}/python -m mapscript.examples.project_csv ${PROJECT_SOURCE_DIR}/tests/cities.csv 2 1 EPSG:4326 EPSG:3857 > test.csv
    COMMAND ${Python_VENV_SCRIPTS}/python -m mapscript.examples.shpdump ${PROJECT_SOURCE_DIR}/tests/polygon.shp > shpdump.txt
    COMMAND ${Python_VENV_SCRIPTS}/python -m mapscript.examples.shpinfo ${PROJECT_SOURCE_DIR}/tests/polygon.shp > shpinfo.txt
    COMMAND ${Python_VENV_SCRIPTS}/python -m mapscript.examples.wxs ${PROJECT_SOURCE_DIR}/tests/test.map > response.xml
    COMMAND ${CMAKE_COMMAND} -E touch ${MAPSCRIPT_WORKING_DIR}/mapscripttests.stamp
    COMMENT "Installing the Python wheel and testing it in the virtual environment, then running tests and examples"
)

add_custom_command(
    TARGET ${SWIG_MODULE_pythonmapscript_REAL_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<SHELL_PATH:${PROJECT_SOURCE_DIR}/src/mapscript/python/mapscript/__init__.py>
        $<SHELL_PATH:$<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/__init__.py>
    COMMAND ${CMAKE_COMMAND} -E copy
        $<SHELL_PATH:${CMAKE_CURRENT_BINARY_DIR}/mapscript.py>
        $<SHELL_PATH:$<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/mapscript.py>
    COMMAND ${CMAKE_COMMAND} -E copy
        $<SHELL_PATH:$<TARGET_FILE:${SWIG_MODULE_pythonmapscript_REAL_NAME}>>
        $<SHELL_PATH:$<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/$<TARGET_FILE_NAME:${SWIG_MODULE_pythonmapscript_REAL_NAME}>>
    COMMAND ${CMAKE_COMMAND} -E copy
        $<SHELL_PATH:${PROJECT_SOURCE_DIR}/src/mapscript/python/README.rst>
        $<SHELL_PATH:$<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/README.rst>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<SHELL_PATH:${PROJECT_SOURCE_DIR}/src/mapscript/python/tests/cases>
        $<SHELL_PATH:$<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/tests>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<SHELL_PATH:${PROJECT_SOURCE_DIR}/tests>
        $<SHELL_PATH:$<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/tests/data>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<SHELL_PATH:${PROJECT_SOURCE_DIR}/src/mapscript/python/examples>
        $<SHELL_PATH:$<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/examples>
    COMMENT "Copying files required to build Mapscript"
)

install(
  CODE "

    SET(ENV{PYTHONPATH} \${Python_SITELIB}:\$ENV{PYTHONPATH})

    if(DEFINED ENV{DESTDIR})
      SET(PYTHON_ROOT \"--root=\$ENV{DESTDIR}\")
    endif()

    if(DEFINED CMAKE_INSTALL_PREFIX)
      SET(PYTHON_PREFIX \"--prefix=\${CMAKE_INSTALL_PREFIX}\")
    endif()

    execute_process(
      COMMAND ${Python_EXECUTABLE} -m pip install --no-build-isolation \${PYTHON_ROOT} \${PYTHON_PREFIX} .
      WORKING_DIRECTORY ${MAPSCRIPT_WORKING_DIR}
    )
  "
)

message(STATUS "CMake Version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Python MapScript output directory: ${MAPSCRIPT_WORKING_DIR}")
message(STATUS "Python Executable: ${Python_EXECUTABLE}")
message(STATUS "Python Version: ${Python_VERSION}")
message(STATUS "Python site packages: ${Python_SITELIB}")
