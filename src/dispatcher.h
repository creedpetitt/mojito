#pragma once

#include <moji/config.h>
#include <moji/middleware.h>
#include <moji/request.h>
#include <moji/response.h>
#include <moji/router.h>

#include <string>
#include <string_view>
#include <vector>

namespace moji {

class Dispatcher {
    const Router& router_;
    const std::vector<Middleware>& middleware_;
    const Config& config_;

public:
    Dispatcher(const Router& router, const std::vector<Middleware>& middleware, const Config& config);

    Async<Response> handle(Request& req, std::string_view client_ip, bool keep_alive) const ;

private:
    Async<void> run_middleware(size_t index, Request& req, Response& res, const Handler& final_handler) const;
};

} // namespace moji
