# Build System

moji is a CMake-first C++23 framework. Applications consume it directly from CMake.

---

## 1. Use moji in an application

The smallest application only needs `moji::core`.

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_api LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    moji
    GIT_REPOSITORY https://github.com/creedpetitt/mojito.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(moji)

add_executable(my_api src/main.cpp)
target_compile_features(my_api PRIVATE cxx_std_23)
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

Configure and build the library using a matching pair of presets:

```bash
cmake --preset debug
cmake --build --preset debug --parallel
```

For dependencies managed by vcpkg, set `VCPKG_ROOT` and use:

```bash
cmake --preset vcpkg-debug
cmake --build --preset vcpkg-debug --parallel
```

The `release` and `vcpkg-release` presets select optimized builds. Each preset
uses its own build directory. These commands build the library, not the examples.

---

## 3. Install and consume as a package

moji installs a CMake package config, so another project can use `find_package`.

```bash
cmake --preset release
cmake --build build/release --parallel
cmake --install build/release --prefix /tmp/moji-install
```

Then consume it:

```cmake
find_package(moji CONFIG REQUIRED)

add_executable(my_api src/main.cpp)
target_compile_features(my_api PRIVATE cxx_std_23)
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

The required dependencies are a C++23 compiler, CMake, threads, and Boost.
`moji_FETCH_BOOST=ON` (the default) uses an existing release source tree supplied
through `moji_BOOST_SOURCE_DIR`, or fetches Boost 1.85. It builds JSON and URL;
System is header-only. `moji_FETCH_BOOST=OFF` requires an installed Boost package,
which can be supplied by the application's vcpkg toolchain.

OpenSSL is optional and controlled separately by `moji_ENABLE_TLS`, which defaults
to `OFF`. An HTTP-only build does not search for or link OpenSSL. The repository's
base vcpkg manifest does not request it.

## 6. Optional TLS dependencies

The build setup for TLS is available before HTTPS sessions are implemented.
Enabling it makes OpenSSL available to moji's implementation; it does not make
the server speak HTTPS yet. Until session support is added, selecting
`NetworkProtocol::HTTPS` throws at startup before opening the listener, even
in a TLS-enabled build. HTTP remains available in both builds.

To configure the repository with OpenSSL acquired through vcpkg:

```bash
cmake --preset vcpkg-debug-tls
cmake --build --preset vcpkg-debug-tls --parallel
```

Use `vcpkg-release-tls` for an optimized build. These presets set
`moji_ENABLE_TLS=ON` and `VCPKG_MANIFEST_FEATURES=tls` before the first `project()`
call. The manifest's optional `tls` feature requests OpenSSL. The regular vcpkg
presets keep TLS off and select no optional manifest features.

Without vcpkg, provide an OpenSSL development installation through the system
package manager or CMake search paths, then configure:

```bash
cmake -S . -B build/tls -DCMAKE_BUILD_TYPE=Debug -Dmoji_ENABLE_TLS=ON
cmake --build build/tls --parallel
```

`find_package(OpenSSL)` discovers an installation; it does not download one.
Fetching Boost does not fetch OpenSSL.

When embedding moji in another application, set `moji_ENABLE_TLS=ON` before
adding moji. The parent application must provide OpenSSL and, when using vcpkg,
declare it in its own manifest. moji's presets and manifest features are not
automatically applied to the parent project.

### Adding the HTTPS implementation

CMake defines the private compiler macro `MOJI_ENABLE_TLS` as `0` or `1` for
moji's source files. Use `#if`, rather than `#ifdef`, because it is defined in
both builds:

```cpp
#if MOJI_ENABLE_TLS
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>

using TlsStream = boost::beast::ssl_stream<boost::beast::tcp_stream>;
// TLS-specific implementation and session instantiation belong in this branch.
#endif
```

Guard every OpenSSL-dependent include and reference, including private header
members and explicit template instantiations. Keep these types in private
headers and implementation files so ordinary consumers do not need the macro.
`moji::core` already links `OpenSSL::SSL` when enabled; that target also carries
the crypto dependency. Applications linking `moji::core` need no manual SSL
linker flags.

When implementing HTTPS, replace the temporary TLS-enabled startup rejection in
`Server::init()` with context setup and TLS session selection. Retain rejection
of HTTPS in builds with TLS disabled. Add the handshake, TLS shutdown, and
`HttpSession<TlsStream>` instantiation inside the enabled branch; the SSL context
must outlive the sessions that use it.

Installed TLS-enabled moji packages declare OpenSSL through
`find_dependency(OpenSSL COMPONENTS SSL)`, regardless of how Boost was obtained.
Consumers must provide that OpenSSL dependency. Installed HTTP-only packages do
not search for OpenSSL.
