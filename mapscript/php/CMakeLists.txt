FIND_PACKAGE(PHP REQUIRED)
INCLUDE_DIRECTORIES(${PHP_INCLUDE_PATH})

if(WIN32)
add_definitions(/DWIN32 /DZEND_WIN32 /DPHP_WIN32 /DZEND_DEBUG=0)

if(NOT CMAKE_CL_64)
add_definitions(-D_USE_32BIT_TIME_T)
endif(NOT CMAKE_CL_64)
if(WITH_THREAD_SAFETY)
add_definitions(-DZTS=1)
endif(WITH_THREAD_SAFETY)
ENDIF(WIN32)

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
   point.c labelleader.c result.c owsrequest.c cluster.c
   layer.c map.c php_mapscript_util.c php_mapscript.c mapscript_i.c
)


if(WIN32)
target_link_libraries(php_mapscript ${MAPSERVER_LIBMAPSERVER} ${PHP_LIBRARY})
ELSE(WIN32)
target_link_libraries(php_mapscript ${MAPSERVER_LIBMAPSERVER})
ENDIF(WIN32)

set_target_properties(php_mapscript PROPERTIES PREFIX "")
install(TARGETS php_mapscript DESTINATION ${PHP_EXTENSION_DIR})
