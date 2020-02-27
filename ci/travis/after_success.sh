#!/bin/sh
set -eu

if [ "$BUILD_NAME" != "PHP_7.2_WITH_ASAN" ]; then
    # Only run coverage when it is safe to do so (not on pull requests), and only on master branch
    echo "$TRAVIS_SECURE_ENV_VARS"
    echo "$TRAVIS_BRANCH"
    sh -c 'if test "$TRAVIS_SECURE_ENV_VARS" = "true" -a "$TRAVIS_BRANCH" = "master"; then echo "run coverage"; ./run_code_coverage_upload.sh; fi'
    ln -s ../../../mapparser.y build/CMakeFiles/mapserver.dir/
    ln -s ../../../maplexer.l build/CMakeFiles/mapserver.dir/
    coveralls --exclude renderers --exclude mapscript --exclude apache --exclude build/mapscript/mapscriptJAVA_wrap.c --exclude build/mapscript/mapscriptPYTHON_wrap.c --exclude shp2img.c --exclude legend.c --exclude scalebar.c --exclude msencrypt.c --exclude sortshp.c --exclude shptreevis.c --exclude shptree.c --exclude testexpr.c --exclude sym2img.c --exclude testcopy.c --exclude shptreetst.c --exclude tile4ms.c --exclude proj --exclude swig-3.0.12 --extension .c --extension .cpp
fi
