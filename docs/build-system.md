# Build and installation

moji provides a static CMake target, `moji::core`. Choose one Boost provider and, independently, whether to enable TLS.

## Requirements

- CMake **3.21+**. The project and version-3 presets use features available at that version. Preset-file `include` is not required.
- A C++23 compiler **and compatible standard library**. The core uses `std::expected`, concepts, coroutines, and floating-point `std::from_chars`. The examples use iostreams instead of requiring `std::print`.
- Git for Git-based FetchContent/checkout instructions. A first source-dependency download needs network access.
- Ninja for the supplied presets. Direct configuration can use another generator.
- Boost 1.85+, supplied through one of the paths below.
- OpenSSL development headers/libraries **only for a TLS-enabled build**.

Use a recent GCC/libstdc++ or Clang/libc++ combination with those library features. Do not assume an older distro's default compiler is sufficient because it accepts a language flag. Select the compiler on the first configuration, for example `-DCMAKE_CXX_COMPILER=g++-15` if that compiler is installed. Changing toolchains or generators is best done in a separate build directory.

Linux and macOS are the intended platforms. This guide does not claim a tested version matrix for every compiler/distro. On macOS, Xcode command-line tools, a sufficiently recent compiler/library, CMake, and Ninja or Make must be available. Package managers can provide build tools; there is no supported automatic OS-package installer in this workflow.

## Build this checkout

All commands in this section run from the repository root.

```sh
cmake --preset release
cmake --build --preset release --parallel
```

The default provider downloads Boost 1.85 and builds its JSON and URL libraries. Boost.System is header-only. You do not manually download Boost modules.

To reuse an existing **release source tree** rather than download one:

```sh
cmake --preset release -Dmoji_BOOST_SOURCE_DIR=/absolute/path/to/boost_1_85_0
cmake --build --preset release --parallel
```

That variable refers to source files, not an installed `include/` directory.

For a plain CMake invocation without Ninja or presets:

```sh
cmake -S . -B build/local -DCMAKE_BUILD_TYPE=Release
cmake --build build/local --parallel
```

Configuration generates build files; `cmake --build` compiles them. The variable selecting an optimized build is `CMAKE_BUILD_TYPE`, not `CMAKE_BUILD_VERSION`.

## Presets

| Configure/build preset | Boost provider | TLS | Build directory |
|---|---|---|---|
| `debug` | Source | Off | `build/debug` |
| `release` | Source | Off | `build/release` |
| `vcpkg-debug` | Installed via toolchain | Off | `build/vcpkg-debug` |
| `vcpkg-release` | Installed via toolchain | Off | `build/vcpkg-release` |
| `vcpkg-debug-tls` | Installed via toolchain | On | `build/vcpkg-debug-tls` |
| `vcpkg-release-tls` | Installed via toolchain | On | `build/vcpkg-release-tls` |

Set `VCPKG_ROOT` to your existing vcpkg checkout before using vcpkg presets. CMake's manifest integration acquires the dependencies listed in `vcpkg/vcpkg.json`. The TLS presets additionally select its `tls` feature, which requests OpenSSL.

```sh
cmake --preset vcpkg-release-tls -Dmoji_BUILD_EXAMPLES=ON
cmake --build --preset vcpkg-release-tls --target 02_https --parallel
```

The root build normally produces the library only. `moji_BUILD_EXAMPLES=ON` adds the examples; `02_https` is added only when TLS is enabled. Build commands and executable paths are in [examples](../examples/README.md).

## Build settings

| Setting | Default | Meaning |
|---|---|---|
| `moji_FETCH_BOOST` | `ON` | Use a supplied/fetched Boost release source tree |
| `moji_BOOST_SOURCE_DIR` | Empty | Existing release source tree used by the source provider |
| `moji_ENABLE_TLS` | `OFF` | Find/link OpenSSL and compile TLS support |
| `moji_BUILD_EXAMPLES` | `OFF` | Add educational application targets |
| `moji_INSTALL` | `ON` at the top level; `OFF` when embedded | Generate installation/package rules |
| `CMAKE_BUILD_TYPE` | Set by presets | Debug or Release for single-configuration generators |
| `CMAKE_CXX_COMPILER` | Toolchain default | Compiler selected on the first configuration |
| `CMAKE_INSTALL_PREFIX` | CMake default | Installation location |
| `CMAKE_PREFIX_PATH` | CMake search paths | Where installed dependency/package configs can be found |

These are cached configuration settings. After enabling examples in a directory,
pass `-Dmoji_BUILD_EXAMPLES=OFF` if a later build should contain only the library.

For vcpkg, `CMAKE_TOOLCHAIN_FILE`, `VCPKG_MANIFEST_DIR`, and `VCPKG_MANIFEST_FEATURES` must be available before the first `project()` call. Presets provide them in time. CMake list-valued cache settings use strings: `"tls"`, or `"feature1;feature2"`, not JSON arrays.

`MOJI_ENABLE_TLS` is a **private compiler definition**, set to `0` or `1` by CMake. Users select the CMake option; they do not manually add that macro to an application. The language standard requirement propagates through `moji::core`. No special Asio macro or LTO setting is required.

## Use a source checkout from another project

A local copy can be added directly:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_api LANGUAGES CXX)

add_subdirectory(external/moji)
add_executable(my_api main.cpp)
target_link_libraries(my_api PRIVATE moji::core)
```

Alternatively, use FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(
    moji
    GIT_REPOSITORY https://github.com/Creed-Petitt/http_server.git
    GIT_TAG feature/clean-up
)
FetchContent_MakeAvailable(moji)
```

This is the refactor's development branch. Pin a reviewed commit/compatible release for repeatable builds; legacy tags can expose different APIs. For an unmerged local change, use the local checkout rather than assuming it has already been published remotely.

The default source provider requires no vcpkg setup. Do not combine its bundled Boost targets with Boost targets already supplied by a parent; choose `moji_FETCH_BOOST=OFF` in that case.

## Use vcpkg or another installed Boost package

For an embedding application, set `moji_FETCH_BOOST=OFF` before adding moji, or pass it on the configure command line. The package provider uses:

```cmake
find_package(Boost 1.85 CONFIG REQUIRED COMPONENTS headers system json url)
```

It works with vcpkg or another complete Boost installation discoverable through CMake package search paths. moji does not detect and adopt vcpkg just because it exists on disk.

A parent application's root `vcpkg.json` can contain:

```json
{
  "name": "my-api",
  "version-string": "0.1.0",
  "dependencies": ["boost-beast", "boost-json", "boost-url"]
}
```

Then configure the application, not the moji subdirectory:

```sh
cmake -S . -B build/vcpkg \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -Dmoji_FETCH_BOOST=OFF \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/vcpkg --parallel
```

For HTTPS, also request `"openssl"` in the parent's manifest and set `moji_ENABLE_TLS=ON`. The parent owns its toolchain/manifest; moji's repository presets and optional manifest features are not automatically merged into it.

## Install from source

```sh
cmake --preset release -Dmoji_INSTALL=ON -Dmoji_BUILD_EXAMPLES=OFF
cmake --build --preset release --parallel
cmake --install build/release --prefix "$HOME/.local"
```

Bundled builds install the required archives and Boost headers, with dependency targets named `moji::boost_*`. External-Boost builds install moji and declare the Boost package dependency instead. TLS-enabled installed packages also declare OpenSSL regardless of the Boost provider.

Consume the installed package:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_api LANGUAGES CXX)
find_package(moji CONFIG REQUIRED)
add_executable(my_api main.cpp)
target_link_libraries(my_api PRIVATE moji::core)
```

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/.local"
cmake --build build --parallel
```

`find_package` discovers an installation; it does not install one. External Boost and OpenSSL dependencies must be provided by the consuming environment/toolchain.

The old `install.sh` is a legacy entry point and is not the supported installation procedure for this refactor. Use the explicit configure/build/install commands above. OS bootstrap/diagnostic tooling is deferred.

## TLS without vcpkg

```sh
cmake -S . -B build/tls \
  -DCMAKE_BUILD_TYPE=Release \
  -Dmoji_ENABLE_TLS=ON \
  -Dmoji_BUILD_EXAMPLES=ON
cmake --build build/tls --target 02_https --parallel
```

Provide an OpenSSL development installation through the system package manager or CMake search paths, optionally `OPENSSL_ROOT_DIR`. Fetching Boost does **not** fetch OpenSSL. With TLS disabled, moji does not search for or link it. See [HTTPS](https.md) for certificates and runtime configuration.
