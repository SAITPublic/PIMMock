# PIMMock Library

## Introduction

This library is a mock-up of the PIM functionality of Samsung's PIM SDK. 

It is designed as a drop-in replacement for Samsung's `PIMLibrary`, 
for use as a development tool on non-ROCm enabled machines. 

The public interface of this library (`pim_runtime_api.h`) is identical 
to the public interface of Samsung's `PIMLibrary`. However, under the 
hood, this library does NOT perform any actual PIM operations, but 
rather emulates the functionality on the host CPU. All memory allocations,
independent of the specified `MEM_TYPE_*`, happen in host RAM and all 
PIM operations are executed on the host CPU. As such, the performance of 
operations, in particular on larger data-sets, may be limited.

## Setup

### Prerequisites
The library is mostly self-contained, the only requirements are CMake 
version >= 3.14.3, a C++-17 enabled compiler, and ninja.

```bash
# CMake 
# Check the version 
cmake --version
# If CMake version < 3.14.3
wget https://github.com/Kitware/CMake/releases/download/v3.24.1/cmake-3.24.1.tar.gz
tar zxf cmake-3.24.1.tar.gz
cd cmake-3.24.1/
./bootstrap
make -j
sudo make install

# C++-17 enable compiler 
apt-get install gcc-8
apt-get install gcc++-8

# ninja 
apt install ninja-build
```


### Setup of PIMMock 
```bash
# Clone the repository
git clone https://github.com/SAITPublic/PIMMock.git 

# Initialize the submodules
git submodule update --init

# Create a build directory
mkdir build
cd build

# Run CMake and build
cmake -G Ninja ..
# If you want to use C/CXX compiler installed, below script will be helpful 
# cmake -G Ninja -DCMAKE_CXX_COMPILER=/usr/bin/g++-8 -DCMAKE_C_COMPILER=/usr/bin/gcc-8 -DCMAKE_INSTALL_PREFIX=$BASE_DIR/pimmock/install ..
ninja

# Run the tests to verify the build
ctest

# Install the library
ninja install
```

## Intellectual Property

### Samsung

To act as drop-in replacement of the original Samsung `PIMLibrary`,
this library uses some IP files from Samsung, concretely:

* `include/pim_runtime_api.h` and `include/pim_data_types.h` are unmodified
  copies of the same headers in the `PIMLibrary`. The original files can
  be found in folder `runtime/include` of `PIMLibrary`.

* The tests in `test/pim/` are modified copies of the tests in folder
  `examples/hip` of the `PIMLibrary`.

* The functions in `test/pim/test_utilities.h` were extracted from the header
  `runtime/include/utility/pim_dump.hpp` in `PIMLibrary`.

* The tests in `test/pim/` use the original test data of `PIMLibrary`. Due
  to the size of the files, the whole `PIMLibrary` is embedded as Git submodule
  in `external/PIMLibrary` and the test-data from `external/PIMLibrary/test_vectors`
  is used for the tests.

For license details, see LICENSE-pimmock

### Others

For half float (FP16) support, the header-only library 
[`half.hpp`](http://half.sourceforge.net/) is used, the header lives in 
`external/half-float`. For license details, see LICENSE-half

The tests use Google Test, which is embedded as Git submodule in `external/googletest`.
