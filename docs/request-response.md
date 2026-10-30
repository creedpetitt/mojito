# Request, response, and JSON

Handlers receive references to the current request and response. The session keeps them alive until processing completes. Do not retain those references, or string views into them, in work that outlives the request; copy the data you need.

## Request basics

| API | Result |
|---|---|
| `method()` | HTTP method as a string view |
| `path()` | Encoded path without the query |
| `body()` | Raw body as a string view |
| `get_header(name)` | First matching value, or an empty view |
| `has_header(name)` | Whether a header exists |
| `headers()` | All stored headers, including duplicates |
| `param(name)` / `param<T>(name)` | Captured route value / converted value |
| `query(name)` / `query<T>(name)` | Optional query value / converted value |
| `query_or<T>(name, fallback)` | Value or fallback when absent |
| `params()` / `query_params()` | Captured parameter / query maps |

Header lookup is case-insensitive. `get_header()` alone cannot distinguish a missing header from a present empty one; use `has_header()` if that distinction matters. The setter helpers `set_header` and `add_header` replace the first matching header or append a new header respectively.

```cpp
app.get("/search/:category", [](const moji::Request& req) {
    return moji::Json{
        {"category", std::string(req.param("category"))},
        {"page", req.query_or<int>("page", 1)},
        {"agent", std::string(req.get_header("User-Agent"))}
    };
});
```

## Parameter conversion and absence

For `query<int>("page")`:

| Input | Result |
|---|---|
| No `page` parameter | `std::nullopt` |
| `page=0` | An optional containing zero |
| `page=12` | An optional containing 12 |
| `page=oops` or `page=` | HTTP 400 exception |

`query_or<int>("page", 1)` uses its fallback for absence, not invalid input. Missing route parameters throw HTTP 400 rather than returning an optional.

Conversion supports `std::string`, booleans, integer types, and floating-point types. Numbers must consume the entire string; leading/trailing whitespace is not silently trimmed. Boolean true spellings are `true`, `1`, `yes`, and `t`; false spellings are `false`, `0`, `no`, and `f`, with case-sensitive comparisons.

Boost.URL validates the target and decodes query names/values. Query `+` becomes a space, while `%2B` becomes a plus. Duplicate decoded keys use the last value. A bare query key without `=` is ignored by the current policy; `name=` is a present empty value. Malformed percent escapes are rejected.

## Request-local state

```cpp
req.set_state("trace_id", std::string("example-123"));
auto trace = req.state<std::string>("trace_id");
```

Values are owned through `std::any`. Reading state returns a copy of the stored value. Read it with the same stored type; an absent key or wrong type returns an empty optional. For example, storing a string literal stores a pointer type, so use `std::string` when you want an owned string.

State lasts for this request only. It is not an application service registry or automatically synchronized shared storage. `expose_client_ip(true)` populates a string under `"client_ip"`.

## JSON input

```cpp
app.post("/users", [](const moji::Request& req) {
    auto body = req.json();
    auto name = body["name"].as<std::string>();
    auto age = body.try_get<int>("age").value_or(0);
    return moji::Json{{"name", name}, {"age", age}};
});
```

`Request::json()` parses the body and throws `BadRequest` on invalid JSON. It does not require or validate a particular Content-Type header. To handle parsing explicitly:

```cpp
app.post("/checked", [](const moji::Request& req, moji::Response& res) {
    auto parsed = req.try_json(); // std::expected<Json, ParseError>
    if (!parsed) {
        res.bad_request(parsed.error().message);
        return;
    }
    res.json(*parsed);
});
```

`Json::parse(text)` exposes the same value-or-error result independently of a request. `ParseError` contains a `code` and `message`.

## Working with Json

```cpp
moji::Json data{
    {"name", "Ada"},
    {"active", true},
    {"roles", moji::Json::array("reader", "editor")}
};

data.set("name", "Grace");
std::string name = data["name"].as<std::string>();
std::string encoded = data.dump();
```

`operator[]` returns a detached `Json` value. Use `set(key, value)` to modify an object; assignment to `data["name"]` does not mutate `data`. Missing keys, out-of-range indexes, or indexing the wrong container return null JSON. A default-constructed `Json` is null; calling `set()` on a non-object replaces it with an object.

Available inspection includes `type()`, `is_null()`, `is_bool()`, `is_number()`, `is_string()`, `is_array()`, `is_object()`, `has(key)`, `size()`, and `empty()`. Size counts object members or array elements; it is zero for other kinds.

Supported `as<T>()` specializations are `bool`, `std::string`, `int`, `unsigned int`, `std::int64_t`, `std::uint64_t`, `double`, and `Json`. Most rejected types/ranges throw `BadRequest`. `try_as<T>()` catches `BadRequest` and returns an empty optional; `try_get<T>(key)` combines lookup and conversion. For scalar types, a missing/null field therefore produces an empty optional. `as<Json>()` preserves any JSON value, including null.

One current numeric limitation: `as<double>()` expects a JSON floating-point value such as `1.5`. Applying it to an integer such as `1` lets a Boost exception escape, which `try_as<double>()` does not catch. The wrapper does not currently provide general numeric coercion.

Object/array construction uses `Json::object({...})`, `Json::array({...})`, or the variadic `Json::array(...)` helper. There is no automatic reflection/model macro or arbitrary-struct serialization API.

## Response construction

```cpp
app.post("/created", [](moji::Response& res) {
    res.created("/users/42")
       .header("X-Example", "moji")
       .json({{"id", 42}});
});
```

Most response methods return `Response&` for chaining:

| API | Behavior |
|---|---|
| `status(code)` | Set status; default is 200 |
| `header` / `set_header` | Replace first matching header or add it |
| `add_header` | Append a header, allowing duplicates |
| `headers({...})` | Set multiple headers |
| `send(std::string)` | Set body and clear file selection |
| `json(Json)` | Serialize JSON and set `Content-Type: application/json` |
| `json_raw(text)` | Set raw body and JSON content type without validating JSON |
| `file(path)` | Select an individual file response |
| `redirect(url, code=302)` | Set status and Location; does not generate a body |
| `created(location="")` | Set 201 and optionally Location |
| `accepted()` | Set 202 |
| `no_content()` | Set 204 and clear body/file selection |

`send()` does not infer a MIME type. Set `Content-Type` yourself when required. It also does not clear previously set headers.

`add_header()` retains duplicate names in the response object, but the current
network writer applies them using Beast's `set()`: repeated names collapse on
the wire. Separate repeated fields such as multiple Set-Cookie headers are a
current writer limitation, not something this helper currently guarantees.

`bad_request(message)`, `unauthorized(message)`, `forbidden(message)`, and `not_found(message)` set the corresponding status and a JSON error body. Alternatively, throw the HTTP exception types described in [routing](routing.md).

The session applies keep-alive framing and the configured Server header. Calling response methods prepares data; it does not immediately write to the socket. Inspection helpers include `get_status`, `get_body`, `get_headers`, `is_file`, and `get_file_path`. `take_body()` moves out body storage. `build_response()` is a separate string-formatting helper, not the session's Beast-based network writer.

## Individual files

```cpp
app.get("/manual", [](moji::Response& res) {
    res.header("Content-Type", "application/pdf").file("assets/manual.pdf");
});
```

Paths are relative to the process working directory unless absolute. The writer opens the file and uses Beast's file-body response path; an open failure produces 404. This is not a guarantee of kernel zero-copy I/O, automatic MIME detection, or nonblocking filesystem access.

There is no built-in static-directory middleware or URL-to-directory mapping. That is on the [roadmap](roadmap.md). Use application-controlled file paths rather than passing an unvalidated request path directly to `file()`.
