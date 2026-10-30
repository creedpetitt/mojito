#include "listener.h"

#include "boost/beast/core/error.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

namespace moji {

    Listener::Listener(
        net::any_io_executor executor,
        const tcp::endpoint& endpoint,
        ExecutorProvider next_executor,
        AcceptHandler on_accept)
        : acceptor_(std::move(executor)),
          next_executor_(std::move(next_executor)),
          on_accept_(std::move(on_accept)) {

    beast::error_code ec;

    ec = acceptor_.open(endpoint.protocol(), ec);
    if (ec) throw std::runtime_error("Acceptor open failed: " + ec.message());

    ec = acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) throw std::runtime_error("Acceptor reuse_address failed: " + ec.message());

    ec = acceptor_.bind(endpoint, ec);
    if (ec) throw std::runtime_error("Acceptor bind failed: " + ec.message());

    ec = acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) throw std::runtime_error("Acceptor listen failed: " + ec.message());
}

void Listener::run() {
    accept_connection();
}

void Listener::accept_connection() {
    acceptor_.async_accept(
        next_executor_(),
        beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
}

void Listener::on_accept(const beast::error_code& ec, tcp::socket socket) {
    if (ec) {
        if (ec == net::error::operation_aborted) {
            return;
        }
        if (ec != net::error::bad_descriptor && ec != net::error::invalid_argument) {
            std::cerr << "accept error: " << ec.message() << std::endl;
        }
    } else {
        on_accept_(std::move(socket));
    }
    if (acceptor_.is_open()) {
        accept_connection();
    }
}

void Listener::stop() noexcept {
    beast::error_code ec;
    ec = acceptor_.close(ec);
}

} // namespace moji
