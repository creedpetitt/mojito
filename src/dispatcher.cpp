#include "dispatcher.h"

#include <moji/exceptions.h>

#include <utility>

namespace moji {

namespace {
    void not_found_handler(Request&, Response& resp) {
        resp.status(404).send("404 Not Found\n");
    }
        
    const Handler& not_found() {
        static const Handler handler = not_found_handler;
        return handler;
    }
} // namespace

Dispatcher::Dispatcher(
    const Router& router,
    const std::vector<Middleware>& middleware,
    const Config& config) : router_(router), middleware_(middleware), config_(config) {}

// Loop though middleware vector and transform req/res in steps
Async<void> Dispatcher::run_middleware(
    size_t index,
    Request& req,
    Response& res,
    const Handler& final_handler) const {
    if (index < middleware_.size()) {
        const auto& mw = middleware_.at(index);
        co_await mw(req, res, [this, index, &req, &res, &final_handler]() -> Async<void> {
            co_await run_middleware(index + 1, req, res, final_handler);
        });
    } else {
        if (const auto* sync = std::get_if<SyncHandler>(&final_handler)) {
            (*sync)(req, res);
        } else {
            co_await std::get<AsyncHandler>(final_handler)(req, res);
        }
    }
}

Async<Response> Dispatcher::handle(
    Request& req, const std::string_view client_ip, 
    const bool keep_alive) const {
        
    Response res;
    try {
        if (config_.expose_client_ip) {
            req.set_state("client_ip", std::string(client_ip));
        }
        std::optional<MatchedRoute> match = router_.match(req.method(), req.path());

        const Handler* handler = nullptr;
        if (match.has_value()) {
            req.set_route_params(std::move(match->params));
            handler = match->handler;
        } else {
            handler = &not_found();
        }
        
        co_await run_middleware(0, req, res, *handler);
        
    } catch (const HttpError& e) {
        res.status(e.status())
           .json({
            {"error", "HTTP Error"},
            {"message", e.what()}
        });
    } catch (const std::exception& e) {
        res.status(500)
           .json({
            {"error", "Internal Server Error"},
            {"message", e.what()}
        });
    }

    res.header("Connection", keep_alive ? "keep-alive" : "close");
    res.header("Server", config_.server_name);

    co_return res;
}

} // namespace moji
