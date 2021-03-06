# hypertrie
A flexible data structure for low-rank, sparse tensors supporting slices by any dimension and einstein summation (einsum)

## build
### prerequisites

install conan, cmake (3.13+) and a C++17 compiler. The steps below are tested for gcc/g++ 8 and 9.

add conan remotes
```shell script
conan remote add tsl https://api.bintray.com/conan/tessil/tsl && conan remote add public-conan https://api.bintray.com/conan/bincrafters/public-conan && conan remote add stiffstream https://api.bintray.com/conan/stiffstream/public
```
and create a conan profile
 ```shell script
conan profile new --detect default
conan profile update settings.compiler.libcxx=libstdc++11 default
 ```

### build

```shell script
mkdir build
cd build
conan install .. --build=missing
cmake ..
```

# running tests
To enable test, set `hypertrie_BUILD_TESTS` in cmake:
```shell script
cmake -Dhypertrie_BUILD_TESTS=ON ..
make -j tests
tests/tests
```
Some tests are using [pytorch](https://github.com/pytorch/pytorch) which is not provided with the code.
Those tests are disabled by default. 
To enable them, provide the path to the pytorch library via cmake variable `hypertrie_LIBTORCH_PATH`.
Prebuild binaries may be download via https://pytorch.org/get-started/locally/ (works at least with Stable|Linux|LibTorch|C++|None).
```shell script
cmake -Dhypertrie_BUILD_TESTS=ON -Dhypertrie_LIBTORCH_PATH=/path/to/libtorch ..
```
