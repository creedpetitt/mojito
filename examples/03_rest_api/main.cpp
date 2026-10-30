#include "users.h"

#include <atomic>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

int main() {
    try {
        auto config = moji::ConfigBuilder()
            .port(8081)
            .threads(4)
            .max_body_size(64 * 1024)
            .build();

        auto store = std::make_shared<example::UserStore>();
        store->create("Ada");

        moji::App app{config};
        auto next_id = std::make_shared<std::atomic<std::uint64_t>>(1);
        app.use([next_id](moji::Request& req, moji::Response& res, moji::Next next)
                -> moji::Async<void> {
            const auto id = next_id->fetch_add(1, std::memory_order_relaxed);
            req.set_state("request_id", id);
            res.header("X-Request-Id", std::to_string(id));
            co_await next();
        });
        example::register_user_routes(app, store);

        std::cout << "Users API: http://localhost:8081 (user 1 is Ada)\n";
        app.init();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
