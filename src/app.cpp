#include <moji/app.h>

#include <memory>
#include <functional>

#include "dispatcher.h"
#include "server.h"

namespace moji {

App::App() {
    dispatcher_ = std::make_unique<Dispatcher>(router_, middleware_, config_);
    server_ = std::make_unique<Server>(*dispatcher_, config_);
}

App::App(Config config)
    : config_{std::move(config)} {
    dispatcher_ = std::make_unique<Dispatcher>(router_, middleware_, config_);
    server_ = std::make_unique<Server>(*dispatcher_, config_);
}

App::~App() = default;

void App::spawn(Async<void> task) const {
    server_->spawn(std::move(task));
}

void App::stop() noexcept {
    auto current = state_.load();
    for (;;) {
        if (current == AppState::Stopping || current == AppState::Stopped) {
            return;
        }

        const auto next = current == (AppState::Configuring)
            ? AppState::Stopped
            : AppState::Stopping;

        if (state_.compare_exchange_weak(current, next)) {
            if (next == AppState::Stopping) {
                server_->stop();
            }
            return;
        }
    }
}

void App::init() {
    auto expected = AppState::Configuring;
    if (!state_.compare_exchange_strong(expected, AppState::Running)) {
        throw std::logic_error("App::init: App has already started or was stopped.");
    }

    try {
        server_->init(config_.port, config_.threads, [this] { stop(); });
    } catch (...) {
        state_.store(AppState::Stopped);
        throw;
    }
    state_.store(AppState::Stopped);
}

void App::use(const Middleware &mw) {
    ensure_configuring("Cannot add middleware");
    middleware_.push_back(mw);
}

} // namespace moji
