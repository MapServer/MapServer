#!/bin/bash -eu
# Copyright 2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################

BUILD_SH_FROM_REPO="$SRC/MapServer/fuzzers/build.sh"
if test -f "$BUILD_SH_FROM_REPO"; then
    if test "$0" != "$BUILD_SH_FROM_REPO"; then
        echo "Running $BUILD_SH_FROM_REPO"
        exec "$BUILD_SH_FROM_REPO"
        exit $?
    fi
fi

apt-get install -y libsqlite3-dev liblzma-dev sqlite3
apt-get remove -y libproj15 libproj-dev libxml2-dev libxml2

# Build libxml2 from source (packaged one depends on libicu which is C++)
wget https://gitlab.gnome.org/GNOME/libxml2/-/archive/v2.10.2/libxml2-v2.10.2.tar.gz
tar xvzf libxml2-v2.10.2.tar.gz
cd libxml2-v2.10.2
cmake . -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS="-fPIC" -DCMAKE_CXX_FLAGS="-fPIC"
make -j$(nproc) -s
make install
cd ..

# Build PROJ dependency (we can't use the packaged one because of incompatibilities
# between libstdc++ from clang and the libc++ of clang used by ossfuzz
git clone --depth 1 https://github.com/OSGeo/PROJ proj
cd proj
cmake . -DBUILD_SHARED_LIBS:BOOL=OFF \
      -DENABLE_TIFF:BOOL=OFF \
      -DENABLE_CURL:BOOL=OFF \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DBUILD_APPS:BOOL=OFF \
      -DBUILD_TESTING:BOOL=OFF
make -j$(nproc) -s
make install
cd ..

#Build gdal dependency
pushd $SRC/gdal
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF \
-DGDAL_BUILD_OPTIONAL_DRIVERS:BOOL=OFF -DOGR_BUILD_OPTIONAL_DRIVERS:BOOL=OFF \
-DBUILD_APPS=OFF \
-DCMAKE_EXE_LINKER_FLAGS="-pthread" ../
make -j$(nproc) GDAL
make install
popd


#Build MapServer
cd $SRC/MapServer
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_STATIC=ON \
-DWITH_PROTOBUFC=0 -DWITH_FRIBIDI=0 -DWITH_HARFBUZZ=0 -DWITH_CAIRO=0 -DWITH_FCGI=0 \
-DWITH_GEOS=0 -DWITH_POSTGIS=0 -DWITH_GIF=0 ../
#While using undefined sanitizer, Project cannot compile binary but can compile library.
make -j$(nproc) mapserver_static

#Fuzzers
for fuzzer in mapfuzzer shapefuzzer configfuzzer owsfuzzer sldfuzzer owsmapfuzzer; do
    $CC $CFLAGS -Wall -Wextra -I. -I.. -I/usr/include/gdal/. -I/usr/local/include/libxml2 -I/usr/include/libxml2 -DPROJ_VERSION_MAJOR=6 -c ../fuzzers/${fuzzer}.c

    $CXX $CXXFLAGS $LIB_FUZZING_ENGINE ${fuzzer}.o -o ${fuzzer} \
        -L. -lmapserver_static -lgdal \
        -Wl,-Bstatic -lpng -ljpeg -lfreetype -lproj -lxml2 -lz -lsqlite3 -llzma \
        -Wl,-Bdynamic  -lpthread -ldl -lc++ -lclang_rt.builtins

    cp ${fuzzer} $OUT/
done

cd ..

zip -r $OUT/mapfuzzer_seed_corpus.zip tests/*.map
zip -r $OUT/shapefuzzer_seed_corpus.zip tests/*.shp tests/*.shx tests/*.dbf
zip -r $OUT/configfuzzer_seed_corpus.zip tests/*.conf etc/*.conf msautotest/config/*.conf

# OWS request fuzzer: seed with a few representative OGC query strings.
printf "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities" > /tmp/ows_seed1
printf "SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&LAYERS=l&STYLES=&SRS=EPSG:4326&BBOX=-180,-90,180,90&WIDTH=10&HEIGHT=10&FORMAT=image/png" > /tmp/ows_seed2
printf "SERVICE=WFS&VERSION=2.0.0&REQUEST=GetFeature&TYPENAMES=l" > /tmp/ows_seed3
printf "SERVICE=WMS&VERSION=1.1.1&REQUEST=GetFeatureInfo&LAYERS=l&QUERY_LAYERS=l&STYLES=&SRS=EPSG:4326&BBOX=-180,-90,180,90&WIDTH=10&HEIGHT=10&FORMAT=image/png&INFO_FORMAT=text/plain&X=1&Y=1" > /tmp/ows_seed4
zip -j $OUT/owsfuzzer_seed_corpus.zip /tmp/ows_seed1 /tmp/ows_seed2 /tmp/ows_seed3 /tmp/ows_seed4

# SLD fuzzer: seed with a StyledLayerDescriptor carrying a rule filter, so the
# mutator starts from something that reaches both the SLD and the Filter
# Encoding parsers.
cat > /tmp/sld_seed1 <<'EOF'
<StyledLayerDescriptor version="1.0.0" xmlns="http://www.opengis.net/sld" xmlns:ogc="http://www.opengis.net/ogc">
<NamedLayer><Name>fuzz</Name><UserStyle><FeatureTypeStyle><Rule>
<ogc:Filter><ogc:PropertyIsEqualTo><ogc:PropertyName>p</ogc:PropertyName><ogc:Literal>1</ogc:Literal></ogc:PropertyIsEqualTo></ogc:Filter>
<PolygonSymbolizer><Fill><CssParameter name="fill">#112233</CssParameter></Fill></PolygonSymbolizer>
</Rule></FeatureTypeStyle></UserStyle></NamedLayer></StyledLayerDescriptor>
EOF
cat > /tmp/sld_seed2 <<'EOF'
<StyledLayerDescriptor version="1.0.0" xmlns="http://www.opengis.net/sld" xmlns:ogc="http://www.opengis.net/ogc">
<NamedLayer><Name>fuzz</Name><UserStyle><FeatureTypeStyle><Rule>
<ogc:Filter><ogc:BBOX><ogc:PropertyName>g</ogc:PropertyName><Box><coordinates>-1,-1 1,1</coordinates></Box></ogc:BBOX></ogc:Filter>
<MinScaleDenominator>0</MinScaleDenominator><MaxScaleDenominator>1e6</MaxScaleDenominator>
<LineSymbolizer><Stroke><CssParameter name="stroke-width">2</CssParameter></Stroke></LineSymbolizer>
</Rule></FeatureTypeStyle></UserStyle></NamedLayer></StyledLayerDescriptor>
EOF
zip -j $OUT/sldfuzzer_seed_corpus.zip /tmp/sld_seed1 /tmp/sld_seed2

# OWS dispatch over a full mapfile: input = mapfile text, "deadbeef"
# separator, CGI query string (see fuzzers/owsmapfuzzer.c). Seeds pair a
# handful of representative inline-feature mapfiles with the request types
# that drive the layer-driven code paths (rendering, feature info templating,
# legend, WFS/GML, OAPIF/CQL2, SLD apply).
mkdir -p /tmp/oms_seed_dir
cat > /tmp/oms_seed_dir/map_a.map <<'OMSEOF'
MAP
NAME "fuzz"
IMAGETYPE "png"
SIZE 32 32
EXTENT -180 -90 180 90
UNITS DD
IMAGECOLOR 220 220 220
PROJECTION "init=epsg:4326" END
WEB
  METADATA
    "ows_enable_request" "*"
    "wms_enable_request" "*"
    "wfs_enable_request" "*"
    "ows_srs" "EPSG:4326 EPSG:3857"
    "ows_title" "fuzz"
    "wfs_title" "fuzz"
    "wfs_getfeature_formatlist" "gml2"
  END
END
OUTPUTFORMAT
  NAME "png"
  DRIVER "AGG/PNG"
  MIMETYPE "image/png"
  IMAGEMODE RGBA
  EXTENSION "png"
END
OUTPUTFORMAT
  NAME "gml2"
  DRIVER "TEMPLATE"
  MIMETYPE "text/xml; subtype=gml/2.1.2"
  IMAGEMODE FEATURE
END
LAYER
  NAME "poly"
  TYPE POLYGON
  STATUS ON
  TEMPLATE "fuzz_tmpl.html"
  PROJECTION "init=epsg:4326" END
  PROCESSING "ITEMS=id;name"
  FEATURE
    POINTS -10 -10 10 -10 10 10 -10 10 -10 -10 END
    ITEMS "1;abc"
  END
  CLASSITEM "id"
  CLASS
    NAME "one"
    EXPRESSION "1"
    STYLE
      COLOR 255 0 0
      OUTLINECOLOR 0 0 0
      WIDTH 1
    END
  END
END
LAYER
  NAME "pts"
  TYPE POINT
  STATUS ON
  TEMPLATE "fuzz_tmpl.html"
  PROJECTION "init=epsg:4326" END
  PROCESSING "ITEMS=id;name"
  FEATURE
    POINTS 0 0 5 5 -5 5 END
    ITEMS "2;xyz"
  END
  CLASS
    STYLE
      COLOR 0 0 255
      SIZE 3
    END
  END
END
END
OMSEOF
cat > /tmp/oms_seed_dir/map_b.map <<'OMSEOF'
MAP
NAME "fancy"
IMAGETYPE "png"
SIZE 32 32
EXTENT 0 0 100 100
UNITS METERS
IMAGECOLOR 255 255 255
WEB
  METADATA
    "ows_enable_request" "*"
    "wms_enable_request" "*"
    "ows_srs" "EPSG:4326 EPSG:3857"
    "ows_title" "fancy"
  END
END
PROJECTION "init=epsg:4326" END
OUTPUTFORMAT
  NAME "png"
  DRIVER "AGG/PNG"
  MIMETYPE "image/png"
  IMAGEMODE RGBA
  EXTENSION "png"
END
SYMBOL
  NAME "square"
  TYPE VECTOR
  FILLED TRUE
  POINTS 0 0 1 0 1 1 0 1 0 0 END
END
LAYER
  NAME "clustered"
  TYPE POINT
  STATUS ON
  CLUSTER
    MAXDISTANCE 20
    REGION "ellipse"
  END
  PROCESSING "ITEMS=id"
  FEATURE
    POINTS 1 1 2 2 50 50 51 51 END
    ITEMS "1;2;3;4"
  END
  CLASS
    STYLE
      SYMBOL "square"
      COLOR 200 0 200
      SIZE 5
    END
    LABEL
      TEXT "[cluster:num_features]"
      SIZE 6
      COLOR 0 0 0
    END
  END
END
LAYER
  NAME "grid"
  TYPE LINE
  STATUS ON
  PROJECTION "init=epsg:4326" END
  GRATICULE
    INTERVALS 5 5
    LABELFORMAT "DDMM"
  END
  CLASS
    STYLE
      COLOR 0 0 255
      WIDTH 1
    END
    LABEL
      SIZE 6
      COLOR 0 0 0
    END
  END
END
LAYER
  NAME "heat"
  TYPE POINT
  STATUS ON
  PROCESSING "HEATMAP_RADIUS=10"
  FEATURE
    POINTS 10 10 20 20 30 30 20 25 END
  END
  CLASS
    STYLE
      COLOR 255 0 0
    END
  END
END
END
OMSEOF
printf 'SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=poly,pts&STYLES=&CRS=EPSG:4326&BBOX=-180,-90,180,90&WIDTH=32&HEIGHT=32&FORMAT=image/png' > /tmp/oms_seed_dir/s1
printf 'SERVICE=WMS&VERSION=1.3.0&REQUEST=GetFeatureInfo&LAYERS=poly&QUERY_LAYERS=poly&STYLES=&CRS=EPSG:4326&BBOX=-180,-90,180,90&WIDTH=32&HEIGHT=32&FORMAT=image/png&INFO_FORMAT=text/html&I=5&J=5&FEATURE_COUNT=10' > /tmp/oms_seed_dir/s2
printf 'SERVICE=WMS&VERSION=1.1.1&REQUEST=GetLegendGraphic&LAYERS=poly&FORMAT=image/png&WIDTH=20&HEIGHT=20&SRS=EPSG:4326&BBOX=-180,-90,180,90' > /tmp/oms_seed_dir/s3
printf 'SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities' > /tmp/oms_seed_dir/s4
printf 'SERVICE=WFS&VERSION=1.0.0&REQUEST=GetFeature&TYPENAME=poly&OUTPUTFORMAT=gml2' > /tmp/oms_seed_dir/s5
printf 'SERVICE=OAPIF&REQUEST=GetCollections' > /tmp/oms_seed_dir/s6
printf 'SERVICE=OAPIF&REQUEST=GetItems&COLLECTION=poly&FILTER="id = 1"&LIMIT=10' > /tmp/oms_seed_dir/s7
printf 'SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=poly&STYLES=&CRS=EPSG:4326&BBOX=-180,-90,180,90&WIDTH=32&HEIGHT=32&FORMAT=image/png&SLD_BODY=<StyledLayerDescriptor version="1.0.0"><NamedLayer><Name>poly</Name><UserStyle><FeatureTypeStyle><Rule><PolygonSymbolizer><Fill><CssParameter name="fill">#00ff00</CssParameter></Fill></PolygonSymbolizer></Rule></FeatureTypeStyle></UserStyle></NamedLayer></StyledLayerDescriptor>' > /tmp/oms_seed_dir/s8
printf 'SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=clustered,grid,heat&STYLES=&CRS=EPSG:4326&BBOX=0,0,100,100&WIDTH=32&HEIGHT=32&FORMAT=image/png' > /tmp/oms_seed_dir/s9
printf 'SERVICE=WFS&VERSION=2.0.0&REQUEST=GetFeature&TYPENAMES=poly&FILTER=<Filter><PropertyIsEqualTo><PropertyName>id</PropertyName><Literal>1</Literal></PropertyIsEqualTo></Filter>' > /tmp/oms_seed_dir/s10
for q in s1 s2 s3 s4 s5 s6 s7 s8 s9 s10; do
  for m in map_a map_b; do
    case "$q" in s9) m=map_b;; esac
    case "$q" in s5|s6|s7|s10) m=map_a;; esac
    cat "/tmp/oms_seed_dir/$m.map" > "/tmp/oms_seed_dir/${q}_${m}"
    printf 'deadbeef' >> "/tmp/oms_seed_dir/${q}_${m}"
    cat "/tmp/oms_seed_dir/$q" >> "/tmp/oms_seed_dir/${q}_${m}"
  done
done
find /tmp/oms_seed_dir -maxdepth 1 -name 's*_map_*' -exec zip -j -q $OUT/owsmapfuzzer_seed_corpus.zip {} \;
cp fuzzers/owsmapfuzzer.dict $OUT/owsmapfuzzer.dict 2>/dev/null || cp $SRC/MapServer/fuzzers/owsmapfuzzer.dict $OUT/owsmapfuzzer.dict
