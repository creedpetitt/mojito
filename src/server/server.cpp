#include "server.h"
#include "listener.h"
#include "../http_session.h"
#include "../dispatcher.h"
#include "../signal_handler.h"

#include <cstdio>
#include <exception>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace blaze {

Server::Server(
    Dispatcher& dispatcher,
    const Config& config)
    : dispatcher_(dispatcher),
      config_(config) {}

Server::~Server() {
    stop();
}

void Server::init(const int port, const int num_threads, std::function<void()> request_stop) {
    try {
        const auto address = net::ip::make_address("0.0.0.0");
        const auto endpoint = tcp::endpoint{address, static_cast<unsigned short>(port)};

        listener_ = std::make_shared<Listener>(
            ioc_,
            endpoint,
            [this](tcp::socket socket) {
                if (ioc_.stopped()) {
                    boost::system::error_code ec;
                    socket.close(ec);
                    return;
                }
                auto session = std::make_shared<Session>(
                    dispatcher_,
                    config_,
                    beast::tcp_stream(std::move(socket)));
                session->run();
            });
        listener_->run();

        signals_ = std::make_unique<SignalHandler>(ioc_, [request_stop = std::move(request_stop)](const int signal_number) {
            std::printf("[Blaze] Received signal %d, stopping...\n", signal_number);
            request_stop();
        });
        signals_->start();

        run(num_threads);
    } catch (...) {
        cleanup();
        throw;
    }
    cleanup();
}

void Server::stop() noexcept {
    // Safe from any thread, including before run(). Never restart this context:
    // a shutdown request made during startup must remain effective.
    ioc_.stop();
}

void Server::cleanup() noexcept {
    // Called by init() only after run() has joined every worker, or when
    // startup failed before any workers existed.
    stop();
    if (signals_) {
        signals_->cancel();
        signals_.reset();
    }
    if (listener_) {
        listener_->stop();
        listener_.reset();
    }
}

void Server::spawn(Async<void> task) {
    net::co_spawn(ioc_.get_executor(), std::move(task), net::detached);
}

void Server::run(const int num_threads) {
    std::mutex failure_mutex;
    std::exception_ptr failure;

    auto run_loop = [&] {
        try {
            ioc_.run();
        } catch (...) {
            {
                std::lock_guard lock(failure_mutex);
                if (!failure) {
                    failure = std::current_exception();
                }
            }
            stop();
        }
    };

    std::vector<std::jthread> workers;
    try {
        workers.reserve(num_threads > 1 ? static_cast<size_t>(num_threads - 1) : 0);
        for (int i = 1; i < num_threads; ++i) {
            workers.emplace_back(run_loop);
        }

        run_loop();
    } catch (...) {
        stop();
        throw;
    }

    workers.clear(); // Join before reading failure or cleaning up server objects.
    if (failure) {
        std::rethrow_exception(failure);
    }
}

} // namespace blaze
