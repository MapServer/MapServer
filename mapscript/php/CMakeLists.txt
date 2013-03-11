FIND_PACKAGE(PHP5 REQUIRED)
INCLUDE_DIRECTORIES(${PHP5_INCLUDES})

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

add_definitions(-DCOMPILE_DL=1)
add_library(php_mapscript MODULE
   mapscript_error.c color.c rect.c hashtable.c web.c grid.c error.c
   referencemap.c querymap.c outputformat.c scalebar.c label.c legend.c
   symbol.c style.c image.c class.c projection.c line.c shape.c shapefile.c
   point.c labelcache.c labelleader.c labelcachemember.c result.c owsrequest.c cluster.c
   layer.c map.c php_mapscript_util.c php_mapscript.c mapscript_i.c
)


target_link_libraries(php_mapscript ${MAPSERVER_LIBMAPSERVER})

set_target_properties(php_mapscript PROPERTIES PREFIX "")
install(TARGETS php_mapscript DESTINATION ${PHP5_EXTENSION_DIR})

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mapscript.py DESTINATION ${PYTHON_SITE_PACKAGES})
#install(TARGETS mapscript DESTINATION ${PYTHON_SITE_PACKAGES})
