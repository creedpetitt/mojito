#pragma once

#include <boost/asio/awaitable.hpp>
#include <moji/config.h>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <memory>
#include <optional>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

using tcp = boost::asio::ip::tcp;

namespace moji {
    
    class Dispatcher;

    template <class Stream>
    class HttpSession : public std::enable_shared_from_this<HttpSession<Stream>> {
        Stream stream_;
        beast::flat_buffer buffer_;
        // Reused only after the preceding response write has completed.
        std::string response_buffer_;
        std::optional<http::request_parser<http::string_body>> parser_;
        Dispatcher& dispatcher_;
        const Config& config_;
        std::string client_ip_;

    public:
        HttpSession(
            Dispatcher& dispatcher,
            const Config& config,
            Stream&& stream);

        void run();

    private:
        static net::awaitable<void> run_connection(
            std::shared_ptr<HttpSession<Stream>> self);
        void prepare_read();
        net::awaitable<bool> read_request();
        net::awaitable<bool> handle_request();
        net::awaitable<void> upgrade_to_https();
        void shutdown_session();
        std::string get_client_ip();
    };

} // namespace moji
