<div align="center">
  <img src="docs/assets/logo.png" width = 500>
  <p><b>moji is a high-performance, asynchronous C++20 web framework designed for developer productivity and extreme scalability</b></p>


[![moji CI](https://github.com/Creed-Petitt/moji/actions/workflows/ci.yml/badge.svg)](https://github.com/Creed-Petitt/moji/actions/workflows/ci.yml)
![License](https://img.shields.io/badge/license-MIT-red)
![Platform](https://img.shields.io/badge/platform-linux%20%7C%20macos-red)
![Standard](https://img.shields.io/badge/c%2B%2B-20-red)

</div>

## Features

- **Asynchronous Coroutines**: High-concurrency powered by C++20 `co_await`.
- **Type-Safe Extraction**: Automatic injection of `Path<T>`, `Body<T>`, and `Query<T>`.
- **Explicit Services**: Register and resolve shared services without hiding application wiring.
- **Focused Middleware**: Rate limiting, CORS, static files, and request context.

## Installation

Install moji as a CMake package, or consume it directly with `FetchContent`.

```bash
// YOLO mode
curl -fsSL https://raw.githubusercontent.com/creedpetitt/mojito/main/install.sh | bash
```

> **Note:** The installer (`install.sh`) installs only the core build tools.

## Local Development

Build moji from this repository:

```bash
./build.sh
```

Build and run the test suite:

```bash
./build.sh test
```

## Quick Start

1. **Write** your logic (e.g., in `src/main.cpp`):
   ```cpp
   #include <moji/app.h>

   using namespace moji;

   int main() {
       App app;

       app.get("/hello", [](Response& res) -> Async<void> {
           res.send("Hello from moji");
           co_return;
      });

       app.listen(8080);
   }
   ```

2. **Build and run** with CMake:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --parallel
   ./build/my-api
   ```

## Documentation

### Project Setup
- **[Getting Started](docs/getting-started.md)**: Installation and project setup.
- **[Build System](docs/build-system.md)**: Direct CMake usage and install/package flow.
- **[Configuration](docs/configuration.md)**: The App Builder API, logging, and environment variables.
- **[Architecture & Design](docs/architecture.md)**: Deep dive into the framework's core engine and design patterns.
- **[Testing & Security](docs/testing.md)**: CI/CD, sanitizers, and performance benchmarking.

### Core Framework
- **[Routing & Request Handling](docs/routing.md)**: Methods, parameters, and typed injection.
- **[Services](docs/dependency-injection.md)**: Explicit app-owned service registration.
- **[File Uploads](docs/file-uploads.md)**: Multipart forms and client-side uploads.
- **[Middleware](docs/middleware.md)**: CORS, rate limiting, static files, and the middleware chain.
- **[WebSockets](docs/websockets.md)**: Real-time broadcasting and background tasks.
- **[Core Utilities](docs/utilities.md)**: Zero-allocation tools for strings, parsing, and resilience.v

## Performance

```
Running 5s test @ http://127.0.0.1:16969/sync/direct
8 threads and 500 connections
Thread Stats   Avg      Stdev     Max   +/- Stdev
Latency     1.61ms    1.37ms  58.73ms   87.07%
Req/Sec    39.71k     3.78k   53.60k    71.75%
1584673 requests in 5.03s, 123.92MB read
Requests/sec: 315088.81
Transfer/sec:     24.64MB
```

## Requirements

moji is designed for modern Linux and macOS environments only. We don't support or plan
to offer support for Windows. 

- **Compiler**: C++20 compliant (GCC 11+, Clang 13+, or MSVC 2022+).
- **Engine**: Boost 1.85+ (Automatically managed via CMake).

## License

moji is released under the [MIT License](LICENSE).
