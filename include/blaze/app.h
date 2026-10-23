#pragma once

#include <blaze/router.h>
#include <blaze/middleware.h>
#include <blaze/injector.h>
#include <blaze/json.h>
#include <blaze/config.h>

#include <atomic>
#include <functional>
#include <vector>
#include <memory>
#include <stdexcept>

namespace blaze {

class Server;
class Dispatcher;

/**
 * @brief Represents the lifecycle state of a Blaze application.
 */
enum class AppState {
    Configuring,
    Running, // Startup or event processing is underway; not a readiness signal.
    Stopping,
    Stopped
};

/**
 * @brief The primary entry point for a Blaze application.
 * 
 * The App class manages the internal Boost.Asio engine, application services,
 * and HTTP routing.
 * Configure routes and middleware on one thread before calling init().
 * stop() and view_state() may be called from other threads. The App must
 * outlive init() and every concurrent public call.
 */
class App {
private:
    Router router_;
    const Config config_;
    std::vector<Middleware> middleware_;
    std::unique_ptr<Dispatcher> dispatcher_;
    std::unique_ptr<Server> server_;
    std::atomic<AppState> state_{AppState::Configuring};

public:
    App();

    explicit App(Config config);

    ~App();

    /**
     * @brief Requests immediate shutdown without waiting for workers.
     * init() returns after workers finish and listener/signal cleanup completes.
     * In-flight responses are not drained. Pending operations are released
     * when the App is destroyed.
     */
    void stop() noexcept;

    /**
     * @brief Access the current lifecycle state.
     */
    AppState view_state() const noexcept { return state_.load(); }

    /**
     * @brief Access the application configuration.
     */
    const Config& view_config() const { return config_; }


    /**
     * @brief Registers a GET route.
     *
     * The handler function supports Request&, const Request&, and Response&.
     *
     * @param path The URL path (e.g., "/users/:id").
     * @param handler The callback function or lambda.
     */
    template<typename Func>
    void get(const std::string& path, Func handler) {
        ensure_configuring("Cannot register GET route");
        router_.add_route("GET", path, wrap_handler(handler));
    }

    /** @brief Registers a POST route. */
    template<typename Func>
    void post(const std::string& path, Func handler) {
        ensure_configuring("Cannot register POST route");
        router_.add_route("POST", path, wrap_handler(handler));
    }

    /** @brief Registers a PUT route. */
    template<typename Func>
    void put(const std::string& path, Func handler) {
        ensure_configuring("Cannot register PUT route");
        router_.add_route("PUT", path, wrap_handler(handler));
    }

    /** @brief Registers a DELETE route. */
    template<typename Func>
    void del(const std::string& path, Func handler) {
        ensure_configuring("Cannot register DELETE route");
        router_.add_route("DELETE", path, wrap_handler(handler));
    }

    /** @brief Starts a background task (coroutine) in the event loop. */
    void spawn(Async<void> task) const;

    /**
     * @brief Starts the HTTP server once and blocks until shutdown completes.
     */
    void init();

    /**
     * @brief Registers global middleware.
     */
    void use(const Middleware &mw);

private:
    void ensure_configuring(const std::string& action) const {
        if (state_.load() != AppState::Configuring) {
            throw std::logic_error(action + ": app is no longer configuring.");
        }
    }

    // Takes lambda and converts it into a standard (Request, Response) handler
    template<typename Func>
    Handler wrap_handler(Func handler) {
        using ReturnType = function_traits<Func>::return_type;
        using AsyncInfo = async_result<ReturnType>;

        return [this, handler](Request& req, Response& res) -> Async<void> {
            if constexpr (AsyncInfo::is_async && !std::is_void_v<typename AsyncInfo::type>) {
                using InnerT = AsyncInfo::type;
                InnerT result = co_await inject_and_call(const_cast<Func&>(handler), req, res);

                if constexpr (std::is_convertible_v<InnerT, std::string>) {
                    res.send(result);
                } else {
                    res.json(result);
                }
            } else {
                co_await inject_and_call(const_cast<Func&>(handler), req, res);
            }
        };
    }
};

} // namespace blaze
