# Build System

moji is a CMake-first C++20 framework. Applications consume it directly from CMake.

---

## 1. Use moji in an application

The smallest application only needs `moji::core`.

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_api LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    moji
    GIT_REPOSITORY https://github.com/Creed-Petitt/moji.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(moji)

add_executable(my_api src/main.cpp)
target_compile_features(my_api PRIVATE cxx_std_20)
target_link_libraries(my_api PRIVATE moji::core)
```

Configure and build it with normal CMake:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/my_api
```

---

## 2. Build this repository

Use the local helper for day-to-day development:

```bash
./build.sh
./build.sh test
./build.sh clean
```

The helper is only a thin wrapper around CMake. The equivalent direct commands are:

```bash
cmake -B build/dev -DCMAKE_BUILD_TYPE=Debug -Dmoji_BUILD_TESTS=ON
cmake --build build/dev --parallel
ctest --test-dir build/dev --output-on-failure
```

---

## 3. Install and consume as a package

moji installs a CMake package config, so another project can use `find_package`.

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -Dmoji_BUILD_TESTS=OFF
cmake --build build/release --parallel
cmake --install build/release --prefix /tmp/moji-install
```

Then consume it:

```cmake
find_package(moji CONFIG REQUIRED)

add_executable(my_api src/main.cpp)
target_compile_features(my_api PRIVATE cxx_std_20)
target_link_libraries(my_api PRIVATE moji::core)
```

Configure the consumer with:

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/tmp/moji-install
cmake --build build --parallel
```

---

## 4. Build examples

Examples are standalone consumer projects. Build one directly from its directory:

```bash
cmake -S examples/01_hello_world -B build/examples/01_hello_world
cmake --build build/examples/01_hello_world --parallel
./build/examples/01_hello_world/01_hello_world
```

Each example links moji the same way a local user project can: by adding the repository root as a CMake subdirectory and linking `moji::core`.

---

## 5. Dependency policy

The default framework build is intentionally small:

- Required: C++20 compiler, CMake, threads, and Boost.Beast/Asio headers.
- Bundled by CMake: Boost 1.85 headers plus the small `boost_system` library.
- Not included: database drivers, TLS/HTTPS, OpenSSL, JWT, or a general-purpose crypto API.

Run TLS at the deployment edge with a reverse proxy, gateway, or load balancer. moji focuses on the application framework and plain HTTP backend runtime.
