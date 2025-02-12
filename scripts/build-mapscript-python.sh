#!/bin/bash
set -eu  # Exit on error and treat unset variables as errors

cd "$WORK_DIR"

# Ensure the Python version is provided
if [ -z "${PYTHON_VERSION:-}" ]; then
    echo "Error: Python version not specified. Make sure to pass it as an environment variable."
    exit 1
fi

DEBIAN_FRONTEND=noninteractive 

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

# build the project
ci/ubuntu/build.sh

echo "Done!"