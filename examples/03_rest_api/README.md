# Users API

A small in-memory API using only current moji features. It starts on port 8081 with user 1 named Ada.

- `main.cpp` configures the app, owns shared objects, and installs request-ID middleware.
- `users.h` declares the store and the route-registration function.
- `users.cpp` implements synchronized storage, validation, CRUD routes, and a real asynchronous timer.
- This is application code, not a framework service container or controller API.

From the repository root:

```sh
cmake --preset release -Dmoji_BUILD_EXAMPLES=ON
cmake --build --preset release --target 03_rest_api --parallel
./build/release/examples/03_rest_api/03_rest_api
```

Then, in another terminal:

```sh
curl -i http://localhost:8081/users/1

curl -i http://localhost:8081/users \
  -H 'Content-Type: application/json' \
  -d '{"name":"Grace"}'

curl -i -X PUT http://localhost:8081/users/2 \
  -H 'Content-Type: application/json' \
  -d '{"name":"Grace Hopper"}'

curl -i -X DELETE http://localhost:8081/users/2
curl -i http://localhost:8081/users/2

curl -i 'http://localhost:8081/wait?ms=200'
curl -i 'http://localhost:8081/wait?ms=invalid'
```

On a fresh process these demonstrate 200, 201 with Location, 200, 204, 404, an awaited timer response, and 400. New-user IDs continue increasing during a process; adapt the ID after additional POSTs.

`name` must be a JSON string containing 1–80 bytes. IDs must be positive integers. Request bodies are limited to 64 KiB. `/wait` defaults to 100 ms and accepts 0–1000 ms. `X-Request-Id` is populated by custom middleware.

The CRUD handlers are ordinary synchronous functions because their work is short and in memory. The timer route returns `Async<Json>` and suspends without sleeping a worker thread.

All route closures share one `UserStore`. Its mutex protects the map and ID sequence. The store returns value copies, so a handler does not retain references into mutable storage or hold a lock across an await.

Data disappears on exit. There is no persistence, authentication, list endpoint, client, or production database abstraction in this example. App shutdown remains immediate.
