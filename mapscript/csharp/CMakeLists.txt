FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

FIND_PROGRAM (CSHARP_COMPILER NAMES csc gmcs gmcs2)

IF (CSHARP_COMPILER)
	MESSAGE(STATUS "Found CSharp compiler: ${CSHARP_COMPILER}")
ELSE (CSHARP_COMPILER)
	MESSAGE(FATAL_ERROR "Could not find csharp compiler")
ENDIF (CSHARP_COMPILER)

if (WIN32)
  if (NOT MSVC71)
    if (CMAKE_CL_64)
      set(PLATFORM_TARGET /platform:anycpu)
    else(CMAKE_CL_64)
      set(PLATFORM_TARGET /platform:x86)
    endif(CMAKE_CL_64)
	set (KEYFILE_SPEC /keyfile:${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\mapscript.snk)
  endif (NOT MSVC71)
  if (MSVC_VERSION GREATER 1500)
      set(PLATFORM_TARGET ${PLATFORM_TARGET} /define:CLR4)
  endif (MSVC_VERSION GREATER 1500)
  MESSAGE(STATUS "Cmake build type: ${CMAKE_BUILD_TYPE}")
  if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo") 
      set(PLATFORM_TARGET ${PLATFORM_TARGET} /debug:full)
  endif (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
endif(WIN32)

MARK_AS_ADVANCED(CSHARP_COMPILER)

include_directories(${PROJECT_SOURCE_DIR}/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/mapscript/csharp)
SET (CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}")
SET( CMAKE_SWIG_FLAGS -namespace OSGeo.MapServer ${MAPSERVER_COMPILE_DEFINITIONS} -DWIN32)
SWIG_ADD_MODULE(csharpmapscript csharp ../mapscript.i)

set_target_properties(csharpmapscript PROPERTIES OUTPUT_NAME "mapscript")

SWIG_LINK_LIBRARIES(csharpmapscript ${MAPSERVER_LIBMAPSERVER})


ADD_CUSTOM_COMMAND(TARGET csharpmapscript
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      POST_BUILD
					  COMMAND set MAPSCRIPT_SNK=${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\mapscript.snk          
					  COMMAND set MAPSCRIPT_SNK=%MAPSCRIPT_SNK:/=\\%          
					  COMMAND copy /Y \"%MAPSCRIPT_SNK%\"
					  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /t:library /out:mapscript_csharp.dll ${KEYFILE_SPEC} *.cs ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\config\\AssemblyInfo.cs
					  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:shpdump.exe ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\examples\\shpdump.cs
					  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:drawmap.exe ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\examples\\drawmap.cs
					  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:shapeinfo.exe ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\examples\\shapeinfo.cs
					  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:drawquery.exe ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\examples\\drawquery.cs
					  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /r:System.Drawing.dll /out:getbytes.exe ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\examples\\getbytes.cs
					  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:HTMLtemplate.exe ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\examples\\HTMLtemplate.cs
					  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:RFC24.exe ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\examples\\RFC24.cs
					  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /r:System.Drawing.dll /out:drawmapDirect.exe ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\examples\\drawmapDirect.cs
					  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /r:System.Drawing.dll /out:drawmapDirectPrint.exe ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\examples\\drawmapDirectPrint.cs
                      COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /r:System.Drawing.dll /out:drawmapGDIPlus.exe ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\examples\\drawmapGDIPlus.cs
					  #COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /lib:C:\Windows\Microsoft.NET\Framework\v4.0.30319\WPF /r:mapscript_csharp.dll /r:System.Xaml.dll /r:WindowsBase.dll /r:PresentationCore.dll /out:drawmapWPF.exe ${PROJECT_SOURCE_DIR}\\mapscript\\csharp\\examples\\drawmapWPF.cs
                      COMMENT "Compiling c# source files"
                      )

#get_target_property(LOC_MAPSCRIPT_LIB ${SWIG_MODULE_csharpmapscript_REAL_NAME} LOCATION)
#install(FILES ${LOC_MAPSCRIPT_LIB} DESTINATION lib)

