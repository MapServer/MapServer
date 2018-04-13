FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})
FIND_PACKAGE(PythonInterp)


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
  FIND_PACKAGE(PythonLibs)
endif(NOT PYTHON_INCLUDE_PATH OR NOT PYTHON_LIBRARIES)


INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
include_directories(${PROJECT_SOURCE_DIR}/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/python)
SWIG_ADD_MODULE(pythonmapscript python ../mapscript.i)

SWIG_LINK_LIBRARIES(pythonmapscript ${PYTHON_LIBRARIES} ${MAPSERVER_LIBMAPSERVER})

set_target_properties(${SWIG_MODULE_pythonmapscript_REAL_NAME} PROPERTIES PREFIX "")
set_target_properties(${SWIG_MODULE_pythonmapscript_REAL_NAME} PROPERTIES OUTPUT_NAME _mapscript)


execute_process ( COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(True))" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)

get_target_property(LOC_MAPSCRIPT_LIB ${SWIG_MODULE_pythonmapscript_REAL_NAME} LOCATION)
set(mapscript_files ${LOC_MAPSCRIPT_LIB} ${CMAKE_CURRENT_BINARY_DIR}/mapscript.py)
install(FILES ${mapscript_files} DESTINATION ${PYTHON_SITE_PACKAGES})

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mapscript.py DESTINATION ${PYTHON_SITE_PACKAGES})
#install(TARGETS mapscript DESTINATION ${PYTHON_SITE_PACKAGES})
