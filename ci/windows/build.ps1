# Configure and build MapServer with the MSSQL plugin
param (
    [string]$SDK = "release-1944-x64",
    [string]$VS_VER = "Visual Studio 18 2026"
)
$ErrorActionPreference = "Stop"

$ROOT = (Get-Location).Path -replace "\\", "/"
$SDK_PREFIX = "$ROOT/sdk/$SDK"
$SDK_INC = "$SDK_PREFIX/include"
$SDK_LIB = "$SDK_PREFIX/lib"
$SDK_BIN = "$SDK_PREFIX/bin"
$REGEX_DIR = "$ROOT/sdk/support/regex-0.12"

New-Item -ItemType Directory -Force build | Out-Null
Push-Location build

$env:PATH = "$ROOT/build/Release;$SDK_BIN;" + $env:PATH

cmake -G "$VS_VER" -A x64 "$ROOT" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$SDK_PREFIX" `
    -DPNG_LIBRARY="$SDK_LIB/libpng16_static.lib" `
    -DHARFBUZZ_INCLUDE_DIR="$SDK_INC/harfbuzz" `
    -DFRIBIDI_INCLUDE_DIR="$SDK_INC/fribidi" `
    -DMS_EXTERNAL_LIBS="$SDK_LIB/harfbuzz.lib;$SDK_LIB/uriparser.lib" `
    -DSVG_LIBRARY="$SDK_LIB/libsvg.lib" `
    -DSVGCAIRO_LIBRARY="$SDK_LIB/libsvg-cairo.lib" `
    -DREGEX_DIR="$REGEX_DIR" `
    -DPROTOBUFC_COMPILER="$SDK_BIN/protoc.exe" `
    -DPROTOBUFC_LIBRARY="$SDK_LIB/protobuf-c.lib" `
    -DPROTOBUFC_INCLUDE_DIR="$SDK_INC/protobuf-c" `
    -DPROJ_INCLUDE_DIR="$SDK_INC/proj9" `
    -DPROJ_LIBRARY="$SDK_LIB/proj9.lib" `
    -DWITH_CURL=1 `
    -DWITH_KML=1 `
    -DWITH_SVGCAIRO=1 `
    -DWITH_THREAD_SAFETY=1 `
    -DWITH_SOS=1 `
    -DWITH_CLIENT_WFS=1 `
    -DWITH_CLIENT_WMS=1 `
    -DWITH_PROTOBUFC=1 `
    -DWITH_HARFBUZZ=1 `
    -DWITH_MSSQL2008=1 `
    -DWITH_POSTGIS=0 `
    -DWITH_PYTHON=0 `
    -DWITH_CSHARP=0 `
    -DWITH_PERL=0 `
    -DWITH_PHPNG=0
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

cmake --build . --config Release
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

# check the executable runs and the MSSQL plugin was built
./Release/mapserv.exe -v
if ($LASTEXITCODE -ne 0) { throw "mapserv -v failed" }
if (-not (Test-Path ./Release/msplugin_mssql2008.dll)) { throw "msplugin_mssql2008.dll not built" }

Pop-Location
