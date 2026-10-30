# Custom middleware

Middleware runs around the selected route handler. Register it before `app.init()`:

```cpp
app.use([](moji::Request& req, moji::Response& res, moji::Next next)
        -> moji::Async<void> {
    res.header("X-Service", "my-api");
    co_await next();
});
```

`Next` is a callable returning `Async<void>`. Await it to run the next middleware or final handler. Simply calling `next()` creates an awaitable and does not execute that coroutine.

## Ordering

For middleware A registered before B:

```text
A before → B before → handler → B after → A after
```

```cpp
#include <chrono>

app.use([](moji::Request&, moji::Response& res, moji::Next next)
        -> moji::Async<void> {
    const auto start = std::chrono::steady_clock::now();
    co_await next();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count();
    res.header("X-Response-Time-US", std::to_string(elapsed));
});
```

Code after `co_await next()` runs on normal completion. If a downstream function throws, it is skipped unless you handle that exception. The dispatcher's outer error handler converts HTTP/standard exceptions into error responses.

Middleware is global and also runs for the fallback 404 handler. Route lookup occurs first; middleware does not automatically trigger rematching if it changes request data.

## Stop the chain

Return without awaiting `next()` to bypass the remaining middleware and handler:

```cpp
app.use([](moji::Request& req, moji::Response& res, moji::Next next)
        -> moji::Async<void> {
    if (req.get_header("X-Demo-Token") != "example") {
        res.unauthorized("Missing demo token");
        co_return;
    }
    co_await next();
});
```

This illustrates control flow, not a complete authentication scheme. Middleware must await and complete its downstream call before returning; do not detach `next()`, invoke it concurrently, or retain it beyond the request.

## Pass request-local data

```cpp
app.use([](moji::Request& req, moji::Response&, moji::Next next)
        -> moji::Async<void> {
    req.set_state("source", std::string("middleware"));
    co_await next();
});

app.get("/", [](const moji::Request& req) {
    return moji::Json{
        {"source", req.state<std::string>("source").value_or("unknown")}
    };
});
```

State lookup uses the stored C++ type as well as the string key. Request-local state is separate from data captured by middleware.

The same middleware callable may be invoked by multiple connections concurrently in either execution model. Mutable captured state needs synchronization. A connection strand protects operations on that connection, not all calls to your middleware across the application.

There are currently no built-in CORS, rate-limit, authentication, or static-directory middleware factories. They can be written using `app.use()`; a small built-in collection is planned. See [the middleware example](../examples/README.md) and [roadmap](roadmap.md).
