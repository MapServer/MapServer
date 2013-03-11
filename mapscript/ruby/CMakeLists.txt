FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})
FIND_PACKAGE(Ruby REQUIRED)
INCLUDE_DIRECTORIES(${RUBY_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/ruby)
SWIG_ADD_MODULE(rubymapscript ruby ../mapscript.i)

SWIG_LINK_LIBRARIES(rubymapscript ${RUBY_LIBRARY} ${MAPSERVER_LIBMAPSERVER})

set_target_properties(${SWIG_MODULE_rubymapscript_REAL_NAME} PROPERTIES PREFIX "")
set_target_properties(${SWIG_MODULE_rubymapscript_REAL_NAME} PROPERTIES OUTPUT_NAME mapscript)

#get_target_property(LOC_MAPSCRIPT_LIB ${SWIG_MODULE_mapscript_REAL_NAME} LOCATION)
#set(mapscript_files ${LOC_MAPSCRIPT_LIB} ${CMAKE_CURRENT_BINARY_DIR}/mapscript.py)
#install(FILES ${mapscript_files} DESTINATION ${PYTHON_SITE_PACKAGES})

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mapscript.py DESTINATION ${PYTHON_SITE_PACKAGES})
#install(TARGETS mapscript DESTINATION ${PYTHON_SITE_PACKAGES})
