/**
 * Example 01: Hello World
 *
 * This example demonstrates the most basic moji application.
 * Concepts:
 * - App instance creation
 * - Configuring the application
 * - Basic GET routing
 * - Using the Response object to send text
 * - Starting the server
 */

#include <moji/app.h>
#include <print>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "../../include/moji/config.h"
#include "moji/request.h"

using namespace moji;

namespace {
int environment_number(const char* name, int fallback, int maximum) {
    const char* raw = std::getenv(name);
    if (!raw) return fallback;
    const std::string_view text{raw};
    int value = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size() || value < 1 || value > maximum)
        throw std::invalid_argument(std::string(name) + " is out of range");
    return value;
}
}

int main() {
    try {
    const char* model_env = std::getenv("MODEL");
    const std::string_view model = model_env ? model_env : "per-worker";
    if (model != "per-worker" && model != "shared")
        throw std::invalid_argument("MODEL must be per-worker or shared");

    // Create and configure the application
    moji::Config cfg = moji::ConfigBuilder()
        .server_name("HelloWorldApp/1.0")
        .threads(environment_number("WORKERS", 8, 1024))
        .port(environment_number("PORT", 16969, 65535))
        .execution_model(model == "shared" ? ExecutionModel::SharedContext : ExecutionModel::PerWorker)
        .build();

    App app{cfg};

    // Define a simple route
    // Handlers are lambdas that return Async<void> (a C++20 coroutine)
    app.get("/", [](Response& res) -> Async<void> {
        // Send a plain text response
        res.send("Hello from moji!");
        co_return;
    });

    app.get("/sync", [](Request& req, Response& res) -> void {
        res.send("Hello from moji!");
    });

    app.get("/sync/direct", [](Request& req) -> std::string {
        return "Hello from moji!";
    });

    app.get("/json/async", [](Request& req) -> Async<Json> {
        co_return Json{
            {"message", "Hello from moji"},
            {"ok", true}
        };
    });

    app.get("/json/sync", [](Request& req) -> Json {
        return Json{
            {"message", "Hello from moji"},
            {"ok", true}
        };
    });

    app.get("/api/users/:id", [](Request& req) -> Async<Json> {
        const int id = req.param<int>("id");
        const bool verbose = req.query_or<bool>("verbose", false);

        co_return Json{
            {"id", id},
            {"verbose", verbose},
            {"roles", Json::array("admin", "editor")},
            {"profile", Json{
                {"active", true},
                {"score", 42}
            }}
        };
    });

    app.post("/api/echo", [](Request& req) -> Async<Json> {
        const auto body = req.json();
        const auto name = body["name"].as<std::string>();
        const auto age = body.try_get<int>("age").value_or(0);

        co_return Json{
            {"ok", true},
            {"name", name},
            {"age", age}
        };
    });

    app.post("/api/echo-safe", [](Request& req, Response& res) -> Async<void> {
        auto parsed = req.try_json();
        if (!parsed) {
            res.status(400).json({
                {"error", "Bad Request"},
                {"message", parsed.error().message}
            });
            co_return;
        }

        const auto name = parsed->try_get<std::string>("name").value_or("anonymous");
        res.json({
            {"ok", true},
            {"name", name},
            {"received", *parsed}
        });
        co_return;
    });

    // This call is blocking and starts the internal thread pool
    std::println("Moji benchmark: port={}, workers={}, model={}", cfg.port, cfg.threads, model);
    app.init();

    return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
