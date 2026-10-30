# Getting started

Start with a small HTTP application. The [build guide](build-system.md) explains how to obtain and link `moji::core`, and the [examples guide](../examples/README.md) has complete runnable projects.

## Create an application

```cpp
#include <moji/app.h>

int main() {
    auto config = moji::ConfigBuilder()
        .port(8080)
        .threads(4)
        .build();

    moji::App app{config};

    app.get("/", []() {
        return "Hello from moji!\n";
    });

    app.get("/users/:id", [](const moji::Request& req) {
        return moji::Json{
            {"id", req.param<int>("id")},
            {"verbose", req.query_or<bool>("verbose", false)}
        };
    });

    app.init();
}
```

`ConfigBuilder` creates a value; pass that value to `App`. Register routes and middleware before calling `init()`. Startup binds an IPv4 listener on all local interfaces, not just loopback.

Try the running application:

```sh
curl http://localhost:8080/
curl 'http://localhost:8080/users/42?verbose=true'
```

The second response contains `{"id":42,"verbose":true}`. An invalid integer produces HTTP 400. A missing route produces HTTP 404.

## Choose a handler style

An ordinary function is suitable for an immediate response:

```cpp
app.get("/ready", []() { return "ready\n"; });

app.post("/accepted", [](moji::Response& res) {
    res.accepted().json({{"queued", true}});
});
```

Use a coroutine when you need to await an asynchronous operation. This route uses Asio's timer rather than blocking the thread:

```cpp
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <chrono>

moji::Async<moji::Json> wait_route() {
    const auto executor = co_await boost::asio::this_coro::executor;
    boost::asio::steady_timer timer{executor};
    timer.expires_after(std::chrono::milliseconds(100));
    co_await timer.async_wait(boost::asio::use_awaitable);
    co_return moji::Json{{"waited_ms", 100}};
}

// During route registration:
// app.get("/wait", wait_route);
```

Both styles can coexist. Returning `Async<T>` does not make a blocking operation nonblocking; it must actually use an asynchronous API. See [routing](routing.md).

## Read JSON

```cpp
app.post("/greet", [](const moji::Request& req) {
    auto body = req.json();
    auto name = body["name"].as<std::string>();
    return moji::Json{{"message", "Hello, " + name}};
});
```

```sh
curl -i http://localhost:8080/greet \
  -H 'Content-Type: application/json' \
  -d '{"name":"Ada"}'
```

`req.json()` parses explicitly. It is not automatic model binding. Invalid JSON or an invalid `as<T>()` conversion throws `BadRequest`. The [request/response guide](request-response.md) also covers nonthrowing parsing.

## Start and stop

`app.init()` blocks while the workers run. The server handles SIGINT and SIGTERM, so Ctrl+C requests shutdown. `app.stop()` can also be called from another thread while the app remains alive.

Shutdown is immediate: outstanding requests are not drained. After shutdown, destroy the app or create a new one; the same app cannot be started twice. `view_state()` reports lifecycle state, but `Running` is not a listener-readiness notification.

Configure and register routes on one thread before startup. Handlers can run concurrently, so synchronize mutable state shared between routes. [The users API example](../examples/03_rest_api/README.md) demonstrates this.

Environment variables, controller registration, dependency-injection containers, and logging configuration are not built-in APIs. Applications can use ordinary C++ facilities and helper functions for those jobs.
