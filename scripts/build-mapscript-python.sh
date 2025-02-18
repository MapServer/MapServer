#!/bin/bash
set -eu  # Exit on error and treat unset variables as errors

cd "$WORK_DIR"

# Ensure the Python version is provided
if [ -z "${PYTHON_VERSION:-}" ]; then
    echo "Error: Python version not specified. Make sure to pass it as an environment variable."
    exit 1
fi

export DEBIAN_FRONTEND=noninteractive

apt-get update -y

# https://github.com/pyenv/pyenv/wiki#suggested-build-environment

# general dependencies
apt-get install -y sudo software-properties-common wget

# pyenv dependencies
apt-get install -y \
    build-essential libssl-dev zlib1g-dev \
    libbz2-dev libreadline-dev libsqlite3-dev curl git \
    libncursesw5-dev xz-utils tk-dev libxml2-dev libxmlsec1-dev libffi-dev liblzma-dev

# install pyenv
curl https://pyenv.run | bash
# https://github.com/pyenv/pyenv?tab=readme-ov-file#b-set-up-your-shell-environment-for-pyenv

# set up pyenv environment
export PYENV_ROOT="$HOME/.pyenv"
export PATH="$PYENV_ROOT/bin:$PATH"

# initialize pyenv (for non-interactive shell)
eval "$(pyenv init --path)"
eval "$(pyenv init -)"

# verify installation
pyenv --version

# export CRYPTOGRAPHY_DONT_BUILD_RUST=1 # to avoid issue when building Cryptography python module
pyenv install -s "$PYTHON_VERSION"

# set the global Python version
pyenv global $PYTHON_VERSION

# install build dependencies
ci/ubuntu/setup.sh

cd "$WORK_DIR"

export CC="ccache gcc"
export CXX="ccache g++"

# only build MapServer with the Python MapScript and not PHP, Perl etc.
make cmakebuild_mapscript_python MFLAGS="-j$(nproc)" CMAKE_C_FLAGS="-O2" CMAKE_CXX_FLAGS="-O2" LIBMAPSERVER_EXTRA_FLAGS="-Wall -Werror -Wextra"
# build the wheel and run the Python MapScript test suite
make mspython-wheel

echo "Done!"