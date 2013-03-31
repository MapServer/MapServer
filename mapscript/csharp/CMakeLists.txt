FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

FIND_PROGRAM (CSHARP_COMPILER NAMES csc gmcs gmcs2)

IF (CSHARP_COMPILER)
	MESSAGE(STATUS "Found CSharp compiler: ${CSHARP_COMPILER}")
ELSE (CSHARP_COMPILER)
	MESSAGE(FATAL_ERROR "Could not find csharp compiler")
ENDIF (CSHARP_COMPILER)

MARK_AS_ADVANCED(CSHARP_COMPILER)

include_directories(${PROJECT_SOURCE_DIR}/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/csharp)
SET (CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}")
SET( CMAKE_SWIG_FLAGS -namespace OSGeo.MapServer ${MAPSERVER_COMPILE_DEFINITIONS} -DWIN32)
SWIG_ADD_MODULE(csharpmapscript csharp ../mapscript.i)

SWIG_LINK_LIBRARIES(csharpmapscript ${MAPSERVER_LIBMAPSERVER})

ADD_CUSTOM_COMMAND(TARGET csharpmapscript
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      POST_BUILD
                      COMMAND ${CSHARP_COMPILER}  /t:library /out:mapscript_csharp.dll *.cs config\\AssemblyInfo.cs
                      COMMENT "Compiling c# source files, creating mapscript_csharp.dll"
                      )

get_target_property(LOC_MAPSCRIPT_LIB ${SWIG_MODULE_csharpmapscript_REAL_NAME} LOCATION)
install(FILES ${LOC_MAPSCRIPT_LIB} DESTINATION lib)

