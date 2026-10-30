# Routing and handlers

Register routes on an `App` before startup:

```cpp
app.get("/hello", []() { return "hello\n"; });
app.post("/users", [](const moji::Request& req) { return req.json(); });
app.put("/users/:id", [](moji::Response& res) { res.no_content(); });
app.del("/users/:id", [](moji::Response& res) { res.no_content(); });
```

The public method helpers are `get`, `post`, `put`, and `del`. Registering routes or middleware after configuration has ended throws `std::logic_error`.

## Matching

Matching uses the HTTP method and path segments. Routes are examined in registration order; the first match wins. Register a literal route such as `/users/me` before `/users/:id` if both could match.

A segment beginning with `:` captures one path segment:

```cpp
app.get("/users/:id", [](const moji::Request& req) {
    return moji::Json{{"id", req.param<int>("id")}};
});
```

The current splitter ignores repeated and trailing slashes. Query text does not participate in matching. A method/path with no match returns 404, including a method mismatch; there is no automatic 405 response.

Literal segments compare encoded path text. Captured values are percent-decoded once after splitting, so `a%2Fb` remains one captured segment whose value is `a/b`. A literal `+` in a path remains `+`. This is not general path normalization, a wildcard router, or static-directory mapping.

## Supported arguments

Handlers can request `Request&`, `const Request&`, and `Response&`, in the order their callable declares. They can also take no arguments.

Argument injection supplies these objects only. There are no `Path<T>`, `Body<T>`, `Query<T>`, service-container, or controller-registration APIs. Extract data from the request explicitly.

Ordinary lambdas and ordinary function pointers work. The current function traits do not provide general support for generic lambdas or overloaded call operators.

## Supported returns

| Handler style | Result |
|---|---|
| Ordinary `void` | Handler fills `Response&` |
| Ordinary string-convertible value | Framework calls `res.send(...)` |
| Ordinary `Json` or another `Json`-constructible value | Framework calls `res.json(...)` |
| `Async<void>` | Framework awaits it; handler fills the response |
| `Async<T>` | Framework awaits it, then applies the same result conversion |

```cpp
app.get("/text", []() -> std::string {
    return "ordinary function\n";
});

app.get("/json", []() {
    return moji::Json{{"ok", true}};
});

app.get("/async-json", []() -> moji::Async<moji::Json> {
    co_return moji::Json{{"ok", true}};
});
```

This last route is a coroutine, but does not wait for anything. Coroutines are useful when they await nonblocking operations, as shown in [getting started](getting-started.md).

Synchronous handlers are stored and called as ordinary functions. They still run inside asynchronous connection/dispatch processing on an I/O worker. A synchronous database call or `sleep_for` blocks that worker; declaring the handler a coroutine does not change that.

Void handlers must fill the response themselves. A returned `Response` object is not a supported response-return convention. Arbitrary C++ structs are not automatically serialized; construct a `Json` explicitly.

## Errors

Throw an HTTP exception for the framework's JSON error response:

```cpp
app.get("/users/:id", [](const moji::Request& req) {
    const int id = req.param<int>("id");
    if (id < 1) throw moji::BadRequest("id must be positive");
    return moji::Json{{"id", id}};
});
```

`HttpError` supplies a status and message. Convenience types include `BadRequest`, `Unauthorized`, `Forbidden`, `NotFound`, `InternalServerError`, and `ServiceUnavailable`. Other standard exceptions escaping handlers/middleware become HTTP 500; the current dispatcher includes their `what()` text in its JSON response, so avoid throwing sensitive internal details to clients.

Routing is resolved before middleware runs. Middleware can short-circuit the selected handler, but changing the request path there does not trigger another route lookup.
