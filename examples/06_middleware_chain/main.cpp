#include <moji/app.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

int main() {
    try {
        moji::App app{moji::ConfigBuilder().port(8082).threads(4).build()};

        app.use([](moji::Request&, moji::Response& res, moji::Next next)
                -> moji::Async<void> {
            const auto start = std::chrono::steady_clock::now();
            co_await next();
            const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start).count();
            res.header("X-Response-Time-US", std::to_string(elapsed));
        });

        auto next_id = std::make_shared<std::atomic<std::uint64_t>>(1);
        app.use([next_id](moji::Request& req, moji::Response& res, moji::Next next)
                -> moji::Async<void> {
            const auto id = next_id->fetch_add(1, std::memory_order_relaxed);
            req.set_state("trace_id", id);
            res.header("X-Trace-Id", std::to_string(id));
            co_await next();
        });

        app.get("/", [](const moji::Request& req) {
            return moji::Json{
                {"message", "Custom middleware"},
                {"trace_id", req.state<std::uint64_t>("trace_id").value_or(0)}
            };
        });

        // Returning without awaiting next() prevents the handler from running.
        app.use([](moji::Request& req, moji::Response& res, moji::Next next)
                -> moji::Async<void> {
            if (req.path() == "/blocked") {
                res.forbidden("Stopped by middleware");
                co_return;
            }
            co_await next();
        });
        app.get("/blocked", []() {
            return "This handler is skipped.\n";
        });

        std::cout << "Middleware example: http://localhost:8082\n";
        app.init();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
