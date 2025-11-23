FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

FIND_PROGRAM (CSHARP_COMPILER NAMES csc mcs gmcs gmcs2)

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
	set (KEYFILE_SPEC /keyfile:${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\mapscript.snk)
  endif (NOT MSVC71)
  if (MSVC_VERSION GREATER 1500)
      set(PLATFORM_TARGET ${PLATFORM_TARGET} /define:CLR4)
  endif (MSVC_VERSION GREATER 1500)
  MESSAGE(STATUS "Cmake build type: ${CMAKE_BUILD_TYPE}")
  if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
      set(PLATFORM_TARGET ${PLATFORM_TARGET} /debug:full)
  endif (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
else()
	set (KEYFILE_SPEC -keyfile:${PROJECT_SOURCE_DIR}/src/mapscript/csharp/mapscript.snk)
endif(WIN32)

MARK_AS_ADVANCED(CSHARP_COMPILER)

include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/swiginc)
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/)
include_directories(${PROJECT_SOURCE_DIR}/src/mapscript/csharp)
include_directories(${PROJECT_BINARY_DIR}/src/mapscript/)
SET (CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}")
if (WIN32)
	SET( CMAKE_SWIG_FLAGS -namespace OSGeo.MapServer -DWIN32 ${MAPSERVER_COMPILE_DEFINITIONS})
else()
	SET( CMAKE_SWIG_FLAGS -namespace OSGeo.MapServer ${MAPSERVER_COMPILE_DEFINITIONS})
endif()

SWIG_ADD_LIBRARY(mapscript LANGUAGE csharp SOURCES ../mapscript.i)

# CMake 3.29.0 corrects an issue with earlier CMake versions regarding the
# shared library name (cf https://gitlab.kitware.com/cmake/cmake/-/issues/25405
# / https://gitlab.kitware.com/cmake/cmake/-/merge_requests/8973)
if (WIN32 OR (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.29.0"))
	set_target_properties(mapscript PROPERTIES OUTPUT_NAME "mapscript")
else()
	set_target_properties(mapscript PROPERTIES OUTPUT_NAME "libmapscript")
endif()

target_link_libraries(mapscript PRIVATE ${MAPSERVER_LIBMAPSERVER})




if (WIN32)

	ADD_CUSTOM_COMMAND(TARGET mapscript
						  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
						  POST_BUILD
						  COMMAND set MAPSCRIPT_SNK=${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\mapscript.snk
						  COMMAND set MAPSCRIPT_SNK=%MAPSCRIPT_SNK:/=\\%
						  COMMAND copy /Y \"%MAPSCRIPT_SNK%\"
						  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /t:library /out:mapscript_csharp.dll ${KEYFILE_SPEC} *.cs ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\config\\AssemblyInfo.cs
						  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:shpdump.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\shpdump.cs
						  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:drawmap.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\drawmap.cs
						  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:shapeinfo.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\shapeinfo.cs
						  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:drawquery.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\drawquery.cs
						  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /r:System.Drawing.dll /out:getbytes.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\getbytes.cs
						  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:HTMLtemplate.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\HTMLtemplate.cs
						  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /out:RFC24.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\RFC24.cs
						  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /r:System.Drawing.dll /out:drawmapDirect.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\drawmapDirect.cs
						  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /r:System.Drawing.dll /out:drawmapDirectPrint.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\drawmapDirectPrint.cs
						  COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /r:mapscript_csharp.dll /r:System.Drawing.dll /out:drawmapGDIPlus.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\drawmapGDIPlus.cs
						  #COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /lib:C:\Windows\Microsoft.NET\Framework\v4.0.30319\WPF /r:mapscript_csharp.dll /r:System.Xaml.dll /r:WindowsBase.dll /r:PresentationCore.dll /out:drawmapWPF.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\drawmapWPF.cs
						  COMMENT "Compiling c# source files"
						  )
else()
	ADD_CUSTOM_COMMAND(TARGET mapscript
						WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
						POST_BUILD
						COMMAND cp -f ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/mapscript.snk ./
						COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} -target:library -out:mapscript_csharp.dll ${KEYFILE_SPEC} *.cs ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/config/AssemblyInfo.cs
						COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} -reference:mapscript_csharp.dll -out:shpdump.exe ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/examples/shpdump.cs
						COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} -reference:mapscript_csharp.dll -out:drawmap.exe ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/examples/drawmap.cs
						COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} -reference:mapscript_csharp.dll -out:shapeinfo.exe ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/examples/shapeinfo.cs
						COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} -reference:mapscript_csharp.dll -out:drawquery.exe ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/examples/drawquery.cs
						COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} -reference:mapscript_csharp.dll -reference:System.Drawing.dll -out:getbytes.exe ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/examples/getbytes.cs
						COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} -reference:mapscript_csharp.dll -out:HTMLtemplate.exe ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/examples/HTMLtemplate.cs
						COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} -reference:mapscript_csharp.dll -out:RFC24.exe ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/examples/RFC24.cs
						COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} -reference:mapscript_csharp.dll -reference:System.Drawing.dll -out:drawmapDirect.exe ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/examples/drawmapDirect.cs
						COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} -reference:mapscript_csharp.dll -reference:System.Drawing.dll -out:drawmapDirectPrint.exe ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/examples/drawmapDirectPrint.cs
						COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} -reference:mapscript_csharp.dll -reference:System.Drawing.dll -out:drawmapGDIPlus.exe ${PROJECT_SOURCE_DIR}/src/mapscript/csharp/examples/drawmapGDIPlus.cs
						#COMMAND ${CSHARP_COMPILER} ${PLATFORM_TARGET} /lib:C:\Windows\Microsoft.NET\Framework\v4.0.30319\WPF /r:mapscript_csharp.dll /r:System.Xaml.dll /r:WindowsBase.dll /r:PresentationCore.dll /out:drawmapWPF.exe ${PROJECT_SOURCE_DIR}\\src\\mapscript\\csharp\\examples\\drawmapWPF.cs
						COMMENT "Compiling c# source files"
						)
endif()


