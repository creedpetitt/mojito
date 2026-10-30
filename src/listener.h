#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>

#include <functional>
#include <memory>

namespace moji {

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class Listener : public std::enable_shared_from_this<Listener> {
public:
    using AcceptHandler = std::function<void(tcp::socket)>;
    using ExecutorProvider = std::function<net::any_io_executor()>;

    Listener(
        net::any_io_executor executor, 
        const tcp::endpoint& endpoint,
        ExecutorProvider next_executor, 
        AcceptHandler on_accept);

    void run();
    void stop() noexcept;

private:
    tcp::acceptor acceptor_;
    ExecutorProvider next_executor_;
    AcceptHandler on_accept_;
    void accept_connection();
    void on_accept(const beast::error_code& ec, tcp::socket socket);
};

} // namespace moji
