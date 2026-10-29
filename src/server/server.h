#pragma once

#include <moji/config.h>
#include <moji/async.h>

#include "engine_pool.h"

#include <boost/asio.hpp>

#include <functional>
#include <memory>

namespace moji {

namespace net = boost::asio;

class Listener;
class Dispatcher;
class SignalHandler;

class Server {
    Dispatcher& dispatcher_; // Owned by app, borrowed by server
    const Config& config_; // Owned by app, borrowed by server
    EnginePool engine_; 
    std::shared_ptr<Listener> listener_;
    std::unique_ptr<SignalHandler> signals_;

public:
    Server(
        Dispatcher& dispatcher,
        const Config& config);

    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // App enforces single startup and owns the shutdown state transitions.
    void init(uint16_t port, int num_threads, std::function<void()> request_stop);
    void stop() noexcept;
    void spawn(Async<void> task);

private:
    void run(int num_threads);
    void cleanup() noexcept;
};

} // namespace moji
