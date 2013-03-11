FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})
FIND_PACKAGE(PythonLibs)
FIND_PACKAGE(PythonInterp)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
include_directories(${PROJECT_SOURCE_DIR}/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/python)
SWIG_ADD_MODULE(pythonmapscript python ../mapscript.i pygdioctx/pygdioctx.c)

SWIG_LINK_LIBRARIES(pythonmapscript ${PYTHON_LIBRARIES} ${MAPSERVER_LIBMAPSERVER})

set_target_properties(${SWIG_MODULE_pythonmapscript_REAL_NAME} PROPERTIES PREFIX "")
set_target_properties(${SWIG_MODULE_pythonmapscript_REAL_NAME} PROPERTIES OUTPUT_NAME _mapscript)


execute_process ( COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(True)" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)

get_target_property(LOC_MAPSCRIPT_LIB ${SWIG_MODULE_pythonmapscript_REAL_NAME} LOCATION)
set(mapscript_files ${LOC_MAPSCRIPT_LIB} ${CMAKE_CURRENT_BINARY_DIR}/mapscript.py)
install(FILES ${mapscript_files} DESTINATION ${PYTHON_SITE_PACKAGES})

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mapscript.py DESTINATION ${PYTHON_SITE_PACKAGES})
#install(TARGETS mapscript DESTINATION ${PYTHON_SITE_PACKAGES})
