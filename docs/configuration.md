# Configuration

`ConfigBuilder` constructs a `Config` value. `App` stores its own immutable copy; changing your original config after constructing the app does not reconfigure the server.

```cpp
#include <moji/app.h>
#include <chrono>

auto config = moji::ConfigBuilder()
    .port(8080)
    .threads(4)
    .execution_model(moji::ExecutionModel::PerWorker)
    .max_body_size(1024 * 1024)
    .timeout(std::chrono::seconds(15))
    .shutdown(std::chrono::seconds(5))
    .server_name("my-api/1.0")
    .build();

moji::App app{config};
// Register routes, then app.init().
```

## Settings and defaults

| Builder method | Config field/type | Default | Meaning |
|---|---|---|---|
| `port(uint16_t)` | `port` | `8080` | TCP listening port |
| `threads(int)` | `threads` | `1` | Workers, including the thread calling `init()`; must be at least 1 |
| `execution_model(ExecutionModel)` | `execution_model` | `PerWorker` | Per-worker or shared-context execution |
| `max_body_size(size_t)` | `max_body_size` | `10 * 1024 * 1024` | HTTP request-body limit in bytes |
| `timeout(std::chrono::seconds)` | `timeout_seconds` | `30s` | Stream deadline set before each read and TLS handshake |
| `shutdown(std::chrono::seconds)` | `shutdown_timeout` | `30s` | Deadline for the normal per-connection TLS closing exchange |
| `server_name(string_view)` | `server_name` / `std::string` | `"mojito/0.1.0"` | `Server` response header |
| `expose_client_ip(bool)` | `expose_client_ip` | `false` | Attach socket peer address as `"client_ip"` request state |
| `protocol(NetworkProtocol)` | `protocol` | `HTTP` | Plain HTTP or HTTPS |
| `certificate_chain_file(string_view)` | `certificate_chain_file` / `std::string` | Empty | PEM certificate-chain file for HTTPS |
| `private_key_file(string_view)` | `private_key_file` / `std::string` | Empty | Matching PEM private-key file for HTTPS |

Calling `expose_client_ip()` without an argument currently supplies `false`; use `expose_client_ip(true)` to enable it.

Builder string arguments are copied into owned config strings. Relative certificate/key paths resolve against the process working directory. HTTPS does not automatically select port 443; choose the port explicitly. Port 8443 is convenient for local testing without privileged-port permission.

`build()` returns the config value; most validation/loading occurs when the engine/server is constructed or started. It is not a comprehensive validation API. HTTP ignores the certificate fields; HTTPS rejects empty paths and propagates certificate/key-loading failures.

## Execution models

```cpp
.execution_model(moji::ExecutionModel::PerWorker)
```

Creates one event loop per worker, each run by one thread. Accepted connections are assigned round-robin and stay on that context. Workers are not pinned to CPU cores.

```cpp
.execution_model(moji::ExecutionModel::SharedContext)
```

Creates one event loop run by all workers. The listener and individual connections use strands to serialize their operations. A strand is not a dedicated thread.

Both models share the same application router, middleware, and captured application state. Synchronize mutable data accessed from different connections. See [architecture](architecture.md).

## Timeouts and shutdown

The stream deadline is an I/O deadline, not a preemptive limit on handler CPU time. A blocking handler cannot be interrupted simply because the deadline elapsed. The read deadline is not a separate idle/handler/write configuration; later stream operations use the stream's current deadline.

`shutdown_timeout` bounds TLS shutdown for a connection that reaches its normal close path. It is **not** an application-wide grace period.

`App::stop()` stops the contexts immediately, and workers are joined before `init()` returns. In-flight responses are not drained. Pending operations are released when the app is destroyed. Normal per-connection TLS shutdown and application-wide stop are different paths.

## Peer address and application settings

With `expose_client_ip(true)`:

```cpp
app.get("/peer", [](const moji::Request& req) {
    return moji::Json{
        {"peer", req.state<std::string>("client_ip").value_or("")}
    };
});
```

This is the socket's remote address. It does not interpret `Forwarded` or `X-Forwarded-For` headers from a reverse proxy.

There are no built-in environment-file, log-level, or thread-auto-detection settings. Applications can read environment variables using standard C++ and pass their results to `ConfigBuilder`. The listener currently binds IPv4 on all local interfaces; there is no bind-address builder field.
