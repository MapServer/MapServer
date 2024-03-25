find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})
find_package(Ruby REQUIRED)
include_directories(${RUBY_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/ruby)

if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
    swig_add_library(rubymapscript TYPE MODULE LANGUAGE ruby SOURCES ../mapscript.i)
else ()
    swig_add_module(rubymapscript ruby ../mapscript.i)
endif ()

swig_link_libraries(rubymapscript ${RUBY_LIBRARY} ${MAPSERVER_LIBMAPSERVER})

set_target_properties(${SWIG_MODULE_rubymapscript_REAL_NAME} PROPERTIES PREFIX "")
set_target_properties(${SWIG_MODULE_rubymapscript_REAL_NAME} PROPERTIES OUTPUT_NAME mapscript)

if(APPLE)
    set_target_properties(${SWIG_MODULE_rubymapscript_REAL_NAME} PROPERTIES SUFFIX ".bundle")
endif(APPLE)

set(mapscript_files $<TARGET_FILE:${SWIG_MODULE_rubymapscript_REAL_NAME}>)
execute_process(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "puts RbConfig::CONFIG['sitearchdir']" OUTPUT_VARIABLE RUBY_SITEARCHDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
install(FILES ${mapscript_files} DESTINATION ${RUBY_SITEARCHDIR})
