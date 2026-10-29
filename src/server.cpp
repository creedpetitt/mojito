#include "server.h"
#include "boost/beast/core/tcp_stream.hpp"
#include "listener.h"
#include "http_session.h"
#include "dispatcher.h"
#include "signal_handler.h"

#include <cstdint>
#include <cstdio>
#include <exception>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace moji {

Server::Server(
    Dispatcher& dispatcher,
    const Config& config)
    : dispatcher_(dispatcher),
      config_(config),
      engine_(config.execution_model, config.threads) {}

Server::~Server() {
    stop();
}

void Server::init(const uint16_t port, const int num_threads, std::function<void()> request_stop) {
    try {
        const auto endpoint = tcp::endpoint{tcp::v4(), port};
        
        listener_ = std::make_shared<Listener>(
            engine_.listener_executor(), endpoint,
            [this] {
                return engine_.next_connection_executor();
            },
            [this](tcp::socket socket) {
                if (engine_.context_for_worker(0).stopped()) {
                    boost::system::error_code ec;
                    ec = socket.close(ec);
                    return;
                }
                auto session =
                    std::make_shared<HttpSession<beast::tcp_stream>>(
                        dispatcher_,
                        config_,
                        beast::tcp_stream(std::move(socket)));
                session->run();
            });
        listener_->run();
        signals_ = std::make_unique<SignalHandler>
            (engine_.context_for_worker(0), 
                [request_stop = std::move(request_stop)](const int signal_number) {
            std::printf("[moji] Received signal %d, stopping...\n", signal_number);
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

void Server::run(const int num_threads) {
    std::mutex failure_mutex;
    std::exception_ptr failure;

    auto run_loop = [&](size_t worker) {
        try {
            // Shared Context always returns 0.
            engine_.context_for_worker(worker).run(); 
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
            workers.emplace_back(run_loop, (size_t)i);
        }

        run_loop(0);
    } catch (...) {
        stop();
        throw;
    }

    workers.clear(); // Join before reading failure or cleaning up server objects.
    if (failure) {
        std::rethrow_exception(failure);
    }
}

void Server::spawn(Async<void> task) {
    net::co_spawn(engine_.spawn_task_executor(), std::move(task), net::detached);
}

void Server::stop() noexcept {
    engine_.stop();
}

void Server::cleanup() noexcept {
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

} // namespace moji
