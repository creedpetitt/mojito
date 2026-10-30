# Moji performance investigation: from profiling to 300k observations

> Historical investigation: measurements and command examples describe the fixtures at the time of the experiments. Current educational examples have been simplified. Use the [build guide](build-system.md) and [examples guide](../examples/README.md) for current commands; these measurements are not HTTPS or cross-platform guarantees.

This records the investigation of Moji's HTTP/1.1 hello-world request path: what
we measured, what the profiles showed, which changes were retained, and which
experiment we rejected. It follows the implementation through the later native
`/sync/direct` results of **315,088.81** and **304,004.81 requests/second**.

Those two results crossed the 300k target for that synchronous endpoint. They
are repeated five-second observations, rather than a guarantee of sustained
300k throughput for every endpoint or machine state.

## 1. Starting point and constraints

The initial reported throughput was roughly 220–230k requests/second. Three
recorded native framework runs later in the discussion were approximately
209k, 210k, and 213k. A standalone Beast server varied around 227–253k in the
recorded runs, with an earlier result around 280k reported as well.

That variation mattered. A raw server result was a reference point, not proof
of a fixed Boost.Beast or Asio throughput ceiling.

The important requirements were:

- Retain Beast's incoming HTTP parser, configured request limits, and timed
  stream. Avoid replacing protocol handling merely to improve a toy endpoint.
- Keep Boost HTTP message types out of Moji's public `Request` and `Response`
  interface. Preserve route parameters, queries, headers, bodies, JSON, and
  middleware behavior.
- Support both execution models: per-worker contexts and a shared context.
- Measure release builds and change one meaningful variable at a time.
- Keep negative results rather than assuming less code or fewer abstractions
  must be faster.

An initial fresh build also exposed a configuration setter mismatch:
`concurrency_model` was implemented while the header declared `io_mode`. That
was repaired for an isolated profiling build, and configuration cleanup later
removed the unused I/O-mode option. It was a build/configuration issue, not an
explanation for the measured request throughput.

## 2. How the measurements were made

The machine was an Intel i7-1165G7 with four physical cores and eight logical
CPUs. Both the load generator and server ran on the same machine. Most tests
used eight server workers and HTTP keep-alive.

The recurring native command was:

```sh
wrk -t8 -c500 -d5s http://127.0.0.1:6969/
```

Later runs used longer durations, `--latency`, and sometimes port 16969. The
first exploratory profile used 256 connections and a 12-second load, so its
throughput should not be treated as a controlled comparison against every later
500-connection result.

`-t8` controls **client** threads. It does not configure the server's worker
count. Both settings must be recorded independently. Also, per-worker execution
assigns connections to contexts; it does not pin OS threads to physical cores.

We separated three kinds of evidence:

1. **Unprofiled throughput:** what `wrk` observed without a profiler attached.
2. **CPU accounting:** server process CPU time per completed request, summed over
   its threads and excluding the load generator.
3. **Profiles and counters:** where user-space cycles were spent and how many
   user instructions/cycles were executed per completed request.

The first captures used optimized builds with debug symbols and frame pointers:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE='-O3 -DNDEBUG -g -fno-omit-frame-pointer'
cmake --build build --parallel 2
```

Run configuration commands in the CMake source directory for the executable
being measured. If an example adds the library with `add_subdirectory`, that
example has its own CMake configuration: changing only a separate library build
does not necessarily change the library copy linked into the example.

Later captures used DWARF stacks, allowing the ordinary optimized release binary
to be profiled without requiring frame pointers. A representative procedure is:

```sh
# Warm up the running server first.
wrk -t8 -c500 -d5s http://127.0.0.1:16969/

# In another terminal, substitute the actual server PID.
perf record -e cycles:u -F 199 --call-graph dwarf,8192 \
  -p SERVER_PID -o moji.data -- sleep 15

# Apply load while recording.
wrk -t8 -c500 -d10s --latency http://127.0.0.1:16969/

# Inspect exclusive costs, then callers.
perf report --stdio --no-children -g none -i moji.data
perf report --no-inline --max-stack 24 -i moji.data
```

For hardware counts, `perf stat -e cycles:u,instructions:u -p SERVER_PID -- sleep 15`
can cover a separately measured load interval. Counts divided by completed
requests are approximate if the counter interval includes a short pre/post-load
period. Check counter running percentages and lost samples.

`cycles:u` and `instructions:u` cover **user space**, not kernel execution.
Process CPU accounting includes user and system CPU; it still does not account
for every system-wide networking cost. Profile percentages are not percentages
of endpoint wall-clock latency, and sampled allocation costs are not allocation
counts. Inclusive caller percentages overlap; exclusive percentages do not.

Agent-side socket/process restrictions required native execution outside the
restricted environment. Even then, machine load, frequency and temperature
varied. Profiler overhead also affects throughput. We therefore kept profiled
request rates separate from capacity measurements and used repeated native runs
instead of treating the best burst as the only result.

## 3. The first important finding: outgoing response machinery

The initial code review found several plausible costs:

- Copying Beast request headers into Moji-owned headers.
- Constructing Moji response headers and then copying them into Beast fields.
- Allocating a path-segment vector even for `/`.
- Coroutine layers for connection helpers, dispatch, middleware and handlers.
- Generic executor and timed-stream operation bookkeeping.

The first profile gave a clearer priority:

| Approximate exclusive user-cycle symbol group | Moji | Standalone Beast |
|---|---:|---:|
| Beast response serialization/buffer traversal | 25% | 31% |
| Generic executor copy/move/destruction | 9% | 11% |
| Allocation/free functions | 8% | 5% |

These were heuristic groups of function samples, affected by inlining. They
identified substantial **outgoing** work; they did not establish that Moji's
`Request` wrapper or incoming HTTP parsing caused the main difference.

We also checked the shared-ownership hypothesis. The listener's `make_shared`
happens at startup, and its pending-accept ownership is handled per connection.
The session owner remains in the connection coroutine across keep-alive
requests. That made listener ownership a lower-priority suspect than repeated
request/response work. Reference counting inside streams and executor machinery
was not ruled out by that observation.

## 4. Controlled diagnostic: keep parsing, change response writing

The first standalone experiment kept the request parser, timed stream, workers,
body, response headers and keep-alive behavior. It changed only the ordinary
outgoing encoding/write path:

```text
Original: construct Beast response → http::async_write
Test:     construct response → encode into one string → asio::async_write
```

That initial diagnostic manually formatted a restricted ordinary HTTP response.
It constructed the outgoing bytes every request; it did not cache a completed
response or bypass request parsing.

Two short native pairs produced:

| Median | Original writer | Diagnostic contiguous writer |
|---|---:|---:|
| Requests/second | 235,781 | 312,137 |
| Server CPU microseconds/request | 21.17 | 14.46 |

Response byte comparisons passed in both execution layouts. The roughly 32%
throughput increase and CPU/request reduction were a strong reason to pursue
the outgoing path. These were short smoke tests, not a sustained-capacity claim.

The diagnostic encoder was not adopted as Moji's production HTTP encoder. A
general implementation must handle more than the particular hello-world
response, including framing and special response semantics.

## 5. Retained change: flatten Beast-generated bytes

To preserve Beast's HTTP formatting, Moji's implementation retained the Beast
response and serializer. It collected the serializer's buffers into a single
string and passed that string through the existing timed stream.

```text
Before:
Moji Response → Beast response → serializer's buffer sequence
              → Beast/Asio response-writing operations → timed stream

After:
Moji Response → Beast response → same serializer
              → contiguous outgoing buffer → asio::async_write(timed stream)
```

This explains why the patch looked additive. We added a collection step and
storage while replacing the small-response network-writing branch. We did not
remove the protocol encoder, request parser, or all of the original writer.

Beast can already submit multiple buffers in one system call. The finding was
not that every header previously required a separate packet or syscall. The
contiguous path reduced buffer-sequence traversal and composed-operation work
around the write. For these small responses, that saving outweighed the extra
copy. The remaining serializer traversal still appeared in later profiles.

The experiment was initially gated by the private CMake option
`moji_EXPERIMENTAL_FLAT_WRITE` and compile definition
`MOJI_EXPERIMENTAL_FLAT_WRITE`. This allowed a release A/B without changing
Moji's public configuration or response API.

Native framework runs with the Beast-generated buffered path reached
**260,603.66** and **266,186.30 requests/second**. A follow-up profile placed
serializer/buffer traversal around 10% of sampled user cycles, compared with
roughly 25% in the earlier capture. Because capture conditions differed, that
percentage change was supporting evidence, not an exact speedup calculation.

## 6. Retained change: one outgoing buffer per session

The first framework version allocated a new output string for each response.
The next change retained that string in the session instead.

In [src/http_session.h](../src/http_session.h), the private session now has two
different buffers:

```cpp
beast::flat_buffer buffer_;      // Incoming bytes, including unread leftovers.
std::string response_buffer_;   // Outgoing serialized bytes; reused per connection.
```

The incoming buffer was not redundant. Overwriting it with outgoing data could
destroy a subsequent pipelined request. The two buffers serve different
directions and lifetimes.

In [src/http_session.cpp](../src/http_session.cpp), `flatten_response()` changed
from returning a newly constructed string to filling a caller-owned string:

```cpp
void flatten_response(
    http::response<http::string_body>& response,
    std::string& wire) {
    http::response_serializer<http::string_body> serializer{response};
    wire.clear();
    wire.reserve(256 + response.body().size());

    while (!serializer.is_done()) {
        beast::error_code ec;
        serializer.next(ec, [&](beast::error_code&, const auto& buffers) {
            std::size_t copied = 0;
            for (auto buffer : beast::buffers_range_ref(buffers)) {
                wire.append(static_cast<const char*>(buffer.data()), buffer.size());
                copied += buffer.size();
            }
            if (copied != 0) serializer.consume(copied);
        });
        if (ec) throw boost::system::system_error(ec);
    }
}
```

`clear()` runs before **every** buffered response. It resets the logical size
while retaining capacity; it is neither response caching nor a secure memory
wipe. Every response is serialized again, and Asio receives only its current
logical bytes.

`write_response()` now accepts the session string by reference. After building
the Beast response as before, its small-response branch is:

```cpp
if (res.body().size() <= max_flat_response_body) {
    flatten_response(res, response_buffer);
    co_await net::async_write(
        stream, net::buffer(response_buffer), net::use_awaitable);

    if (response_buffer.capacity() > max_retained_response_buffer)
        std::string{}.swap(response_buffer);
    co_return;
}
```

`HttpSession::handle_request()` passes `response_buffer_` into this helper. The
connection loop awaits each response write before processing the next request,
so the buffer cannot be cleared for the next response while the previous write
still uses it. If writing fails, the existing connection-boundary error handling
ends the session.

The retained policy is:

- Bodies up to **64 KiB** use the contiguous output buffer.
- Capacity above **128 KiB** is released after the completed write, preventing
  unusually large response headers from retaining that allocation indefinitely.
- The retention threshold is not a global memory or response-header limit;
  transient allocations can exceed it.
- Files and larger bodies retain the previous writer, avoiding another full-size
  payload copy. Dedicated `write_error()` responses retain their original writer
  too. Handler-generated small error responses can still use the ordinary
  buffered path.
- `co_return` prevents a small response from falling through and being written
  twice.

The experiment option and compile definition were then removed from
[CMakeLists.txt](../CMakeLists.txt) and the implementation. This is now the default
small-response path, with no macro or public configuration switch required.

## 7. Validation and what reuse actually achieved

Checks performed during implementation included:

- Release library and hello-world builds.
- 80 byte comparisons against Beast's normal writer, covering HTTP versions,
  keep-alive, statuses, empty/binary bodies, headers and chunking.
- 1,000 repeated responses retaining the same output-buffer allocation.
- 23 deferred partial-write comparisons, including the large-body/file fallback,
  missing-file response and retained-capacity cap. These used an asynchronous
  capture stream to test suspension and partial completion without relying on
  live-network timing.
- Live checks for payloads, HTTP/1.0, fragmentation, pipelining, malformed headers
  and request body limits.

The rewrite retained the incoming Beast parser, configured body limit,
`beast::tcp_stream`, deadlines, keep-alive handling and existing shutdown/error
boundaries. It added no Boost HTTP types to the public `Request`/`Response` API.
These checks support the change; they do not constitute a blanket security audit
or prove every possible protocol interaction.

Native reuse results of approximately 245–268k overlapped the earlier 261–266k
results. A follow-up allocation/free sample group decreased from about 14.7% to
13.6%, but separate captures could not establish a throughput gain.

Later hardware counters reinforced the scale of this change: standalone
Beast-generated buffering used about 23.1k user instructions/request with a fresh
string and 22.7k with reused storage, roughly 1.5% less. Reuse removed repeated
allocation of this buffer; it did not remove request, response-body, header or
coroutine-frame allocations throughout the system.

## 8. Synchronous handlers became a genuinely synchronous handler path

During the investigation, handler storage/adaptation also separated synchronous
and asynchronous callables. This was a distinct change from output buffering.

The earlier common handler representation returned `Async<void>` for every
callable, including synchronous user functions. The retained representation in
[include/moji/handler.h](../include/moji/handler.h) is:

```cpp
using AsyncHandler = std::function<Async<void>(Request&, Response&)>;
using SyncHandler = std::function<void(Request&, Response&)>;
using Handler = std::variant<AsyncHandler, SyncHandler>;
```

`App::wrap_handler()` in [include/moji/app.h](../include/moji/app.h) uses the
callable's return type to select the appropriate alternative. Synchronous
functions are invoked directly, and returned strings/JSON are sent through
`send_handler_result()`. Asynchronous functions retain the coroutine adapter and
are awaited. Both void handlers that fill `Response` and handlers returning
values remain supported.

At the end of `Dispatcher::run_middleware()`, the dispatcher calls a
`SyncHandler` directly or awaits an `AsyncHandler`. Networking, connection
handling and the surrounding dispatcher/middleware machinery still use their
existing asynchronous execution. This did not turn a synchronous endpoint into
a blocking socket server.

For immediate work, an async handler returning immediately has no I/O wait to
overlap. Avoiding its handler/adapter coroutine frames can therefore save work.
Later counters showed around 5% fewer user instructions for the sync plaintext
case and around 6% fewer for sync JSON. These were complete route comparisons,
with slightly different lookup positions, not an exact measurement of one frame.

The sync/async JSON routes constructed the same JSON object on every request.
The sync route generally showed an advantage in the native observations. This
does not establish zero JSON overhead: larger bodies, Content-Type handling,
construction and serialization all contribute to endpoint cost.

## 9. Rejected experiment: bypassing the empty middleware coroutine

Once allocation appeared more prominent, we tried a further shortcut in
`Dispatcher::handle()`:

```text
No registered middleware → invoke the matched sync/async handler directly
Registered middleware   → use run_middleware() as before
```

This passed 12 behavior checks covering sync/async invocation, actual async
suspension, middleware order/short-circuiting, not-found handling and exceptions.
We compared saved baseline and candidate libraries linked with the same example
object, compiler flags and payload. Three pairs used an eight-thread,
500-connection, ten-second load with warmup, reversing order in the middle pair.

| Median | Existing chain | Empty-chain bypass |
|---|---:|---:|
| Requests/second | 229,055 | 189,606 |
| Server CPU microseconds/request | 21.51 | 22.09 |

All three pairs favored the existing implementation, so the bypass was reverted.
It is **not present** in the current dispatcher.

Disassembly showed the caller coroutine's frame allocation growing from 464 to
496 bytes. That illustrated why removing a child coroutine does not necessarily
shrink or simplify the generated caller. It did not prove the cause of the
observed regression. Frequency/load drift and compiler/frame-cache behavior
remained possible influences; the measured 17% median gap was not claimed as a
universal intrinsic penalty.

This is separate from retaining ordinary synchronous handlers: rejecting the
middleware shortcut did not undo the sync/async handler variant.

## 10. Execution models and remaining costs

The existing models were tested rather than assumed equivalent:

| Model | Implementation |
|---|---|
| Per-worker | One `io_context(1)` per worker, one thread running each context, round-robin connections, no strands |
| Shared context | All workers run one context; the listener and each connection use strands |

Per-worker remained preferable for this uniform, tiny-response workload. Earlier
observations put its advantage around 10–15%; later runs showed larger gaps under
other conditions. Neither percentage is a universal property. Shared execution
can still be useful to understand and evaluate for uneven workloads. Adding a
database does not automatically make it faster.

Later profiles showed costs spread across allocation, HTTP parsing/fields,
serializer/buffer traversal, executor bookkeeping, coroutine machinery, scheduling,
timers and socket operations. A representative reuse capture grouped roughly:

| User-space symbol family | Approximate share |
|---|---:|
| Other Asio dispatch/timer functions | 14% |
| Allocation/free | 14% |
| Serializer/buffer traversal | 13% |
| Other Beast HTTP functions | 13% |
| Moji functions, including inlined work | 11% |
| Stream/socket/read-write functions | 10% |
| Generic executor bookkeeping | 7% |
| Coroutine/recycling machinery | 6% |
| Mutex functions | 3% |
| Other symbols | 9% |

Those rounded categories explain why a 15% leading group did not mean the other
85% was unaccounted for or impossible to optimize. They also excluded kernel
execution entirely. Asio recycles coroutine frames, but observed allocation
stacks still reached the allocator; recycling did not make every frame free.

Matching standalone Beast to Moji's current writer produced a remaining
framework throughput gap around 10% in one broader comparison. That was useful
evidence that the Request/router wrapper alone was not the original dominant
problem. It was not proof that all remaining framework overhead had been removed.

## 11. The later 300k observations

The direct synchronous route was measured natively at port 16969:

```sh
wrk -t8 -c500 -d5s http://127.0.0.1:16969/sync/direct
```

| Run | Requests completed | Requests/second | Average latency |
|---|---:|---:|---:|
| 1 | 1,584,673 | **315,088.81** | 1.61 ms |
| 2 | 1,531,444 | **304,004.81** | 1.73 ms |

Both runs crossed 300k. They measured a synchronous direct-return handler, not
an async route. The preceding connection refusal at port 6969 meant no listener
was available there; switching to the active port fixed the test target. It was
not a measured throughput failure.

Earlier native observations also included approximately 281k for `/sync` and
291k for `/sync/direct`. Some earlier route bodies differed, so those values
cannot isolate coroutine overhead by themselves. Longer, repeated, identical
loads are still needed for a sustained-capacity claim. These later observations
are retained alongside the lower runs rather than replacing them.

## 12. Current implementation and conclusions

| Area | Retained outcome |
|---|---|
| `src/http_session.cpp` | Beast-generated small responses flattened before Asio writes; existing file, large-body and dedicated-error paths retained |
| `src/http_session.h` | One reusable outgoing string per session, separate from incoming buffering |
| `CMakeLists.txt` | Experimental flat-write option/definition removed; no switch needed |
| `include/moji/handler.h` | Distinct sync/async function alternatives |
| `include/moji/app.h` | Return-type-based handler adaptation without forcing sync functions into a handler coroutine |
| `src/dispatcher.cpp` | Sync alternatives called directly at the end of the existing chain; empty-chain bypass reverted |
| `src/engine_pool.cpp` | Existing per-worker/shared models retained; no new execution model introduced |
| Public request/response features | Route/query extraction, headers, bodies, JSON and middleware preserved |

The server implementation files were separately reorganized from `src/server/`
into `src/`; the paths above use the current layout. That cleanup is not credited
as a measured performance optimization.

The largest demonstrated improvement came from the outgoing write path, not
removing request parsing. Buffer reuse was a smaller allocation improvement.
Synchronous handlers avoided unnecessary handler coroutine work for immediate
responses. The failed middleware shortcut showed why measurement must overrule
an appealing source-level simplification.

At this point, pausing hello-world tuning was reasonable. Remaining experiments
such as compiler/LTO settings, default-header allocation, executor specialization
and request storage reuse were leads, not established wins or changes made in
this investigation. No maximum Beast throughput ceiling was established.

Async handlers will become useful when they **await nonblocking operations**, for
example a compatible asynchronous database driver. A blocking query or heavy
CPU calculation still occupies its worker even inside a coroutine; it must use
an appropriate asynchronous API or be moved to another execution resource.
Both execution models can overlap nonblocking waits. Future database/JSON-list
workloads should be measured on their own merits rather than inferred from
hello-world request rates.
