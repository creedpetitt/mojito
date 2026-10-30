# Examples

These applications use the current public API. They are educational examples rather than a benchmark harness.

| Target / directory | Port | Demonstrates |
|---|---|---|
| `01_hello_world` | 8080 | Minimal text and JSON routes |
| `02_https` | 8443 | HTTPS and certificate/key arguments |
| `03_rest_api` | 8081 | Organized routes, JSON validation, shared state, an async timer |
| `06_middleware_chain` | 8082 | Timing, request state, and short-circuiting |
| `09_background_tasks` | 8083 | `App::spawn` with an Asio timer and an atomic counter |

The HTTPS executable is now called `02_https`, not `02_hello_world`. Old build artifacts are not automatically renamed or removed.

## Build through the root presets

From the repository root:

```sh
cmake --preset release -Dmoji_BUILD_EXAMPLES=ON
cmake --build --preset release --parallel
```

The HTTP examples share the same library build. Without TLS, CMake skips `02_https` and prints a status message. To build only one executable:

```sh
cmake --build --preset release --target 03_rest_api --parallel
./build/release/examples/03_rest_api/03_rest_api
```

Use `debug`, `vcpkg-debug`, or `vcpkg-release` in place of `release` as appropriate. vcpkg presets require `VCPKG_ROOT`.

For all examples including HTTPS:

```sh
cmake --preset vcpkg-debug-tls -Dmoji_BUILD_EXAMPLES=ON
cmake --build --preset vcpkg-debug-tls --parallel
```

Executables are under `build/<preset>/examples/<directory>/<target>` with these single-configuration presets. They are not installed by `cmake --install`.

## Run from the repository root

```sh
./build/release/examples/01_hello_world/01_hello_world
```

In another terminal:

```sh
curl http://localhost:8080/
curl http://localhost:8080/json
```

For middleware:

```sh
./build/release/examples/06_middleware_chain/06_middleware_chain
```

```sh
curl -i http://localhost:8082/
curl -i http://localhost:8082/blocked
```

The first response includes trace/timing headers. The second returns 403 without invoking its route handler.

The background example exposes its timer counter:

```sh
./build/release/examples/09_background_tasks/09_background_tasks
```

```sh
curl http://localhost:8083/
```

The count increases every ten seconds. Ctrl+C requests immediate app shutdown.

For `02_https`, generate local files and pass both paths as described in [HTTPS](../docs/https.md). For `03_rest_api`, see its [walkthrough](03_rest_api/README.md).

## Standalone builds

Each directory also works as a standalone CMake project:

```sh
cmake -S examples/03_rest_api -B build/standalone/03_rest_api \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/standalone/03_rest_api --parallel
./build/standalone/03_rest_api/03_rest_api
```

This builds moji as a subdirectory and only that example, regardless of the root example option. The default source provider acquires Boost automatically.

For a standalone HTTPS example with system-provided OpenSSL:

```sh
cmake -S examples/02_https -B build/standalone/02_https \
  -DCMAKE_BUILD_TYPE=Release \
  -Dmoji_ENABLE_TLS=ON
cmake --build build/standalone/02_https --parallel
./build/standalone/02_https/02_https \
  examples/02_https/certs/cert.pem \
  examples/02_https/certs/key.pem
```

A standalone example has its own CMake cache. Building the library separately with a root TLS preset does not enable TLS in that independent configuration. To use vcpkg with standalone examples, provide the toolchain, `moji_FETCH_BOOST=OFF`, and the absolute manifest directory before configuration; root presets do not automatically apply.

## Organization and limits

The examples register everything before `init()`. Shared state is synchronized even in per-worker mode. They use iostreams, so `std::print` support is not an extra requirement.

The first two applications intentionally contain few routes. More complete patterns live in `03_rest_api` instead of being mixed into hello-world. The old realtime-chat, multipart-upload, and static-site examples are not present; those APIs should not be inferred from older documentation.
