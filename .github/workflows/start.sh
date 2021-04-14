#!/bin/sh

set -e

apt-get update -y

export BUILD_NAME=PHP_7.3_WITH_PROJ7
#export PYTHON_VERSION=3.6
export PYTHON_VERSION=system

LANG=en_US.UTF-8
export LANG
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    sudo locales tzdata software-properties-common python3-dev python3-pip python3-setuptools git curl \
    apt-transport-https ca-certificates gnupg software-properties-common wget \
    php-dev phpunit && \
    echo "$LANG UTF-8" > /etc/locale.gen && \
    dpkg-reconfigure --frontend=noninteractive locales && \
    update-locale LANG=$LANG

USER=root
export USER

# Install pyenv
curl -L https://github.com/pyenv/pyenv-installer/raw/master/bin/pyenv-installer | bash
export PATH="/root/.pyenv/bin:$PATH"
eval "$(pyenv init -)"
eval "$(pyenv virtualenv-init -)"
ln -s /usr/bin/python3 /usr/bin/python
ln -s /usr/bin/pip3 /usr/bin/pip

export CRYPTOGRAPHY_DONT_BUILD_RUST=1 # to avoid issue when building Cryptography python module
pip install --upgrade pip

# Install recent cmake
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'

cd "$WORK_DIR"

ci/travis/before_install.sh
ci/travis/script.sh
