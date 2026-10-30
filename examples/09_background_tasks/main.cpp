#include <moji/app.h>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>

namespace {

moji::Async<void> monitor(std::shared_ptr<std::atomic<std::uint64_t>> ticks) {
    const auto executor = co_await boost::asio::this_coro::executor;
    boost::asio::steady_timer timer{executor};

    for (;;) {
        timer.expires_after(std::chrono::seconds(10));
        co_await timer.async_wait(boost::asio::use_awaitable);
        const auto tick = ticks->fetch_add(1, std::memory_order_relaxed) + 1;
        std::cout << "[monitor] tick " << tick << '\n';
    }
}

} // namespace

int main() {
    try {
        auto ticks = std::make_shared<std::atomic<std::uint64_t>>(0);
        moji::App app{moji::ConfigBuilder().port(8083).threads(2).build()};

        // The coroutine owns its argument and starts when the engine runs.
        app.spawn(monitor(ticks));
        app.get("/", [ticks]() {
            return moji::Json{{"ticks", ticks->load(std::memory_order_relaxed)}};
        });

        std::cout << "Background example: http://localhost:8083\n";
        app.init();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
