find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

find_package (Python REQUIRED COMPONENTS Interpreter Development)

include_directories(${Python_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/python)

set(SwigFile ${PROJECT_SOURCE_DIR}/mapscript/mapscript.i)

# add annotations to py file output - requires >= SWIG 4.1
if (WITH_PYMAPSCRIPT_ANNOTATIONS AND ${Python_VERSION_MAJOR} GREATER_EQUAL 3)
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

set(SETUP_PY_IN "${PROJECT_SOURCE_DIR}/mapscript/python/setup.py.in")
set(SETUP_PY_TEMP "${CMAKE_CURRENT_BINARY_DIR}/setup.py.temp")

configure_file(${SETUP_PY_IN} ${SETUP_PY_TEMP} @ONLY)
file(READ ${SETUP_PY_TEMP} SETUP_CONTENT)

# each target (e.g. Debug, Release etc.) will have a copy of setup.py

file(GENERATE OUTPUT $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/setup.py INPUT ${SETUP_PY_TEMP})

if(MSVC)
    set(OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
else()
    # for non-Windows builds there are no build type subfolders (e.g. Release, Debug etc.)
    set(OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(WIN32)
    # Windows venv binaries are in a different location than Linux
    set(Python_VENV_SCRIPTS ${OUTPUT_FOLDER}/mapscriptvenv/Scripts)
else()
    set(Python_VENV_SCRIPTS ${OUTPUT_FOLDER}/mapscriptvenv/bin)
endif()

add_custom_target(
    pythonmapscript-wheel
    DEPENDS mapscripttests.stamp
)

add_custom_command(
    DEPENDS ${SWIG_MODULE_pythonmapscript_REAL_NAME}
    OUTPUT mapscriptvenv.stamp
    WORKING_DIRECTORY ${OUTPUT_FOLDER}
    COMMAND ${Python_EXECUTABLE} -m pip install pip --upgrade
    COMMAND ${Python_EXECUTABLE} -m pip install virtualenv
    COMMAND ${Python_EXECUTABLE} -m virtualenv mapscriptvenv
    COMMAND ${Python_VENV_SCRIPTS}/pip install -r ${PROJECT_SOURCE_DIR}/mapscript/python/requirements-dev.txt > requires.log
    COMMENT "Creating a Python virtual environment and installing the required packages"
)

add_custom_command(
    DEPENDS mapscriptvenv.stamp
    OUTPUT mapscriptlinting.stamp
    WORKING_DIRECTORY ${OUTPUT_FOLDER}
    COMMAND ${Python_VENV_SCRIPTS}/flake8 $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/tests --max-line-length=120
    COMMAND ${Python_VENV_SCRIPTS}/flake8 $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/examples --max-line-length=120
    COMMENT "Linting test suite and examples with flake8" # note only one comment is output per custom command block
)

add_custom_command(
    DEPENDS mapscriptlinting.stamp
    OUTPUT mapscriptwheel.stamp
    WORKING_DIRECTORY ${OUTPUT_FOLDER}
    COMMAND ${Python_VENV_SCRIPTS}/python setup.py bdist_wheel > wheel_build.log
    COMMENT "Building the mapscript Python wheel"
)

add_custom_command(
    WORKING_DIRECTORY ${Python_VENV_SCRIPTS} # make sure scripts aren't run when from the same folder as mapscript.py
    DEPENDS mapscriptwheel.stamp
    OUTPUT mapscripttests.stamp
    COMMAND ${Python_VENV_SCRIPTS}/pip install --no-index --find-links=${OUTPUT_FOLDER}/dist mapscript
    # ERROR: file or package not found: mapscript.tests (missing __init__.py?) is caused by
    # ImportError: DLL load failed while importing _mapscript: The specified module could not be found.
    COMMAND ${Python_VENV_SCRIPTS}/python -m pytest --pyargs mapscript.tests
    COMMAND ${Python_VENV_SCRIPTS}/python -m mapscript.examples.project_csv ${PROJECT_SOURCE_DIR}/tests/cities.csv 2 1 EPSG:4326 EPSG:3857 > test.csv
    COMMAND ${Python_VENV_SCRIPTS}/python -m mapscript.examples.shpdump ${PROJECT_SOURCE_DIR}/tests/polygon.shp > shpdump.txt
    COMMAND ${Python_VENV_SCRIPTS}/python -m mapscript.examples.shpinfo ${PROJECT_SOURCE_DIR}/tests/polygon.shp > shpinfo.txt
    COMMAND ${Python_VENV_SCRIPTS}/python -m mapscript.examples.wxs ${PROJECT_SOURCE_DIR}/tests/test.map > response.xml
    COMMENT "Installing the Python wheel and testing it in the virtual environment, then running tests and examples"
)

add_custom_command(
    OUTPUT mapscriptcopyfiles.stamp
    WORKING_DIRECTORY ${OUTPUT_FOLDER}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/mapscript/python/mapscript/__init__.py" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/__init__.py
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/mapscript.py" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/mapscript.py
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${SWIG_MODULE_pythonmapscript_REAL_NAME}>" "$<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/$<TARGET_FILE_NAME:${SWIG_MODULE_pythonmapscript_REAL_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/mapscript/python/README.rst" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/README.rst
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/mapscript/python/tests/cases" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/tests
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/tests" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/tests/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/mapscript/python/examples" $<TARGET_FILE_DIR:${SWIG_MODULE_pythonmapscript_REAL_NAME}>/mapscript/examples
    COMMENT "Copying files required to build Mapscript"
    DEPENDS _pythonmapscript
)

# handle CMake CMP0040 policy error
add_custom_target(
    _pythonmapscript
    DEPENDS mapscriptcopyfiles.stamp
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
      COMMAND ${Python_EXECUTABLE} setup.py install \${PYTHON_ROOT} \${PYTHON_PREFIX}
      WORKING_DIRECTORY ${OUTPUT_FOLDER}
    )
  "
)

message(STATUS "CMake Version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Python MapScript output directory: ${OUTPUT_FOLDER}")
message(STATUS "Python Executable: ${Python_EXECUTABLE}")
message(STATUS "Python Version: ${Python_VERSION}")
message(STATUS "Python site packages: ${Python_SITELIB}")
