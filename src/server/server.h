#pragma once

#include <blaze/config.h>
#include <blaze/async.h>

#include <boost/asio.hpp>

#include <functional>
#include <memory>

namespace blaze {

namespace net = boost::asio;

class Listener;
class Dispatcher;
class SignalHandler;

class Server {
    Dispatcher& dispatcher_;
    const Config& config_;
    net::io_context ioc_;
    std::shared_ptr<Listener> listener_;
    std::unique_ptr<SignalHandler> signals_;

public:
    Server(
        Dispatcher& dispatcher,
        const Config& config);

    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // App enforces single startup and owns the shutdown-state transitions.
    void init(uint16_t port, int num_threads, std::function<void()> request_stop);
    void stop() noexcept;
    void spawn(Async<void> task);
    net::io_context& get_engine() { return ioc_; }

private:
    void run(int num_threads);
    void cleanup() noexcept;
};

} // namespace blaze
