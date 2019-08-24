#!/bin/sh
set -eu

export CC="ccache gcc"
export CXX="ccache g++"
make cmakebuild MFLAGS="-j2"
make mspython-wheel
make -j4 test
