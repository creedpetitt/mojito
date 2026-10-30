# moji

A C++23 HTTP/1.1 framework built on Boost.Asio and Boost.Beast. Write ordinary functions for immediate responses or coroutines for asynchronous work, using the same routing API.

Current features:

- GET, POST, PUT, and DELETE routes with named path parameters.
- Request/query conversion, headers, request-local state, and a JSON wrapper.
- Custom asynchronous middleware.
- Per-worker event loops or one shared event loop.
- Optional HTTPS through OpenSSL, plus background coroutines.
- Individual file responses through `Response::file()`.

## A small application

```cpp
#include <moji/app.h>

int main() {
    moji::App app;
    app.get("/", []() { return "Hello from moji!\n"; });
    app.get("/json", []() {
        return moji::Json{{"message", "Hello from moji!"}};
    });
    app.init(); // Listens on port 8080; blocks until stopped.
}
```

## Build the included examples

From the repository root, with a C++23 compiler/standard library, CMake 3.21+, and Ninja:

```sh
cmake --preset release -Dmoji_BUILD_EXAMPLES=ON
cmake --build --preset release --target 01_hello_world --parallel
./build/release/examples/01_hello_world/01_hello_world
```

CMake acquires Boost 1.85 automatically. No separate Boost download is needed. For a vcpkg installation selected through `VCPKG_ROOT`, use `vcpkg-release` instead. The [examples guide](examples/README.md) covers all applications, standalone builds, and HTTPS.

## Use moji from another CMake project

Put the application above in `main.cpp`, then add:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_api LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    moji
    GIT_REPOSITORY https://github.com/Creed-Petitt/http_server.git
    GIT_TAG feature/clean-up
)
FetchContent_MakeAvailable(moji)

add_executable(my_api main.cpp)
target_link_libraries(my_api PRIVATE moji::core)
```

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/my_api
```

These docs describe the current refactor. The example selects its development branch; pin a reviewed commit or compatible release for a reproducible application. Older release tags may have a different API.

The [build guide](docs/build-system.md) also covers a local checkout, vcpkg-managed Boost/OpenSSL, installing from source, and `find_package(moji CONFIG REQUIRED)`. vcpkg is an optional dependency provider; it is not required to use CMake.

## Documentation

| Guide | What it explains |
|---|---|
| [Getting started](docs/getting-started.md) | A first application, startup, and shutdown |
| [Build and installation](docs/build-system.md) | Requirements, flags, presets, dependency providers, packages |
| [Configuration](docs/configuration.md) | Every setting, its type, default, and meaning |
| [Routing](docs/routing.md) | Methods, matching, synchronous and coroutine handlers |
| [Request, response, and JSON](docs/request-response.md) | Reading input and constructing output |
| [Middleware](docs/middleware.md) | Ordering, request state, and short-circuiting |
| [HTTPS](docs/https.md) | Optional TLS, certificates, and local testing |
| [Architecture](docs/architecture.md) | Both execution models, ownership, and request flow |
| [Examples](examples/README.md) | Runnable projects and commands |
| [Roadmap](docs/roadmap.md) | Planned functionality |
| [Performance investigation](docs/performance-investigation.md) | Historical measurements and optimization decisions |

## Requirements and current scope

The core requires C++23 standard-library support, including `std::expected`, and Boost 1.85+. Linux and macOS are the intended platforms; a supported compiler language mode alone does not guarantee that its selected standard library has the required features. HTTPS additionally requires OpenSSL development files; vcpkg can acquire them automatically.

The API uses asynchronous network I/O. A synchronous handler runs on an I/O worker, so blocking calls still block that worker. `App::stop()` is immediate; it does not drain in-flight requests.

The [roadmap](docs/roadmap.md) includes an async client, WebSockets, GoogleTest coverage, CORS middleware, and static-directory serving. These are not current framework APIs.
