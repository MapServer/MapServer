echo "Running Python tests"

# report Python versions
which pip
which python

# install Python dependencies
python install -m pip install pip --upgrade
pip install cryptography==3.4.6 # avoid requiring rust compiler for the cryptography dependency
pip install cpp-coveralls pyflakes lxml pytest
pip install -r msautotest/requirements.txt

cd msautotest

python -m pyflakes .
cd ..

# run the Python server for the tests
python -m http.server &> /dev/null &

# build the wheel and run the Python MapScript test suite
make mspython-wheel

if ! [ "${WITH_ASAN:-}" = "true" ]; then
    # skip Python tests when running WITH_ASAN
    # ASan runtime does not come first in initial library list; you should either link runtime to your application or manually preload it with LD_PRELOAD
    make mspython-testcase
fi
