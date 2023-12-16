# report Python versions
which pip
which python

cd msautotest
python -m pyflakes .
cd ..

# install Python dependencies
pip install cryptography==3.4.6 # avoid requiring rust compiler for the cryptography dependency
pip install cpp-coveralls pyflakes lxml
pip install -r msautotest/requirements.txt

# run the Python server for the tests
python -m http.server &> /dev/null &

# building the wheel also runs the Python MapScript test suite
make mspython-wheel