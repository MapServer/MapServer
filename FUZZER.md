#### Export Symbols
```
export CC=clang
export CXX=clang++
export CFLAGS=-fsanitize=fuzzer-no-link,address
export LIB_FUZZING_ENGINE=-fsanitize=fuzzer
export LDFLAGS=-fsanitize=address 
```

#### Build it
```
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_STATIC=ON -DFUZZER=ON \
-DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX \
-DCMAKE_C_FLAGS=$CFLAGS -DCMAKE_CXX_FLAGS=$CFLAGS \
-DLIB_FUZZING_ENGINE=$LIB_FUZZING_ENGINE \
../
```

#### Run it
```
mkdir mapcoverage/
mkdir shapecoverage/

mkdir input/
cp ../tests/*.map input/

./mapfuzzer mapcoverage/ input/
./shapefuzzer shapecoverage/
```
