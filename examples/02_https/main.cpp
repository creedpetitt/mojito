/**
 * Example 02: HTTPS
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
#include <iostream>
#include <string_view>

using namespace moji;

int main() {
    try {
    // Create and configure the application
    moji::Config cfg = moji::ConfigBuilder()
        .server_name("HelloWorldApp/1.0")
        .threads(8)
        .port(8443)
        .execution_model(ExecutionModel::PerWorker)
        .protocol(NetworkProtocol::HTTPS)
        .certificate_chain_file("certs/cert.pem")
        .private_key_file("certs/key.pem")
        .build();

    App app{cfg};

    // Define a simple route
    // Handlers are lambdas that return Async<void> (a C++20 coroutine)
    app.get("/", [](Response& res) -> Async<void> {
        // Send a plain text response
        res.send("Hello from moji: TLS Edition!!!");
        co_return;
    });

    app.get("/sync", [](Request& req, Response& res) -> void {
        res.send("Hello from moji: HTTPS!!!");
    });

    app.get("/sync/direct", [](Request& req) -> std::string {
        return "Hello from moji!: HTTPS";
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
    std::println("Moji benchmark: port={}, workers={}", cfg.port, cfg.threads);
    app.init();

    return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
