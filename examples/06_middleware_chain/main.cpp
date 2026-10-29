/**
 * Example 06: Middleware Chain
 * 
 * This example demonstrates how to build custom middleware and use the Request Context.
 * Concepts:
 * - Custom middleware with the Next function
 * - Request-scoped data
 * - Request timing
 */

#include <moji/app.h>
#include <moji/middleware.h>

#include <chrono>
#include <ctime>
#include <iostream>
#include <string>

using namespace moji;

int main() {

    auto cfg = moji::ConfigBuilder().threads(8).build();
    
    App app{cfg};

    // 1. Custom Timer Middleware
    app.use([](Request& req, Response& res, Next next) -> Async<void> {
        auto start = std::chrono::steady_clock::now();

        // Pass control to the next middleware or handler
        co_await next();

        auto end = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        
        // Add a custom header to the response
        res.header("X-Response-Time-US", std::to_string(diff));
        co_return;
    });

    // 2. Custom Trace ID Middleware
    app.use([](Request& req, Response& res, Next next) -> Async<void> {
        // Generate a simple trace ID
        std::string trace_id = "trace-" + std::to_string(std::time(nullptr));
        
        req.set_state("trace_id", trace_id);

        co_await next();
        co_return;
    });

    // 3. Handler reading request-scoped data explicitly
    app.get("/", [](Request& req) -> Async<Json> {
        auto trace_id = req.state<std::string>("trace_id").value_or("");
        co_return Json({
            {"message", "Middleware trace demo"},
            {"your_trace_id", trace_id}
        });
    });

    std::cout << "Middleware demo running on :8080" << std::endl;
    app.init();

    return 0;
}
