#include "http_session.h"
#include "dispatcher.h"

#include <moji/exceptions.h>
#include <moji/request.h>
#include <moji/response.h>

#include <boost/beast/http/file_body.hpp>

#include <boost/asio/write.hpp>
#include <boost/beast/core/buffers_range.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/system/system_error.hpp>

#include <exception>
#include <iostream>
#include <string_view>
#include <tuple>
#include <utility>

namespace net = boost::asio;

namespace moji {

template<class Stream>
struct StreamTraits {
    static void shutdown(Stream& stream) {
        beast::error_code ec;
        get_socket(stream).shutdown(tcp::socket::shutdown_send, ec);
    }

    static tcp::socket& get_socket(Stream& stream) {
        return beast::get_lowest_layer(stream).socket();
    }
};

namespace {

struct ErrorResponse {
    http::status status;
    std::string message;
};

// No response is needed for a normal disconnect, timeout, or cancellation.
std::optional<ErrorResponse> read_error_response(const beast::error_code& ec) {
    if (ec == http::error::end_of_stream ||
        ec == net::error::connection_reset || ec == net::error::eof ||
        ec == beast::error::timeout || ec == net::error::operation_aborted) {
        return std::nullopt;
    }

    if (ec == http::error::body_limit) {
        return ErrorResponse{http::status::payload_too_large, "Payload Too Large"};
    }

    std::cerr << "Request Parse Error: " << ec.message() << "\n";
    return ErrorResponse{http::status::bad_request, "Bad Request"};
}

// Classify only failures escaping the dispatcher; unknown exceptions propagate
// to the connection boundary. A system error closes without another write.
std::optional<ErrorResponse> handler_error_response(std::exception_ptr error) {
    try {
        std::rethrow_exception(error);
    } catch (const HttpError& e) {
        return ErrorResponse{static_cast<http::status>(e.status()), e.what()};
    } catch (const boost::system::system_error& e) {
        if (e.code() != net::error::operation_aborted) {
            std::cerr << "Async Handler Error: " << e.what() << "\n";
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "Async Handler Error: " << e.what() << "\n";
        return ErrorResponse{http::status::internal_server_error, "Internal Server Error"};
    }
}

Request make_request(http::request<http::string_body>&& req) {
    Request moji_req;
    moji_req.set_method(req.method_string());
    moji_req.set_target(std::string_view(req.target().data(), req.target().size()));
    moji_req.set_body(std::move(req.body()));

    for (const auto& field : req) {
        moji_req.add_header(field.name_string(), field.value());
    }

    return moji_req;
}

// Keep HTTP framing in Beast; only change the buffers passed to the timed stream.
// Bound additional body buffering and leave large/file responses on the old path.
constexpr std::size_t max_flat_response_body = 64 * 1024;
constexpr std::size_t max_retained_response_buffer = 128 * 1024;

void flatten_response(
    http::response<http::string_body>& response,
    std::string& wire) {
    http::response_serializer<http::string_body> serializer{response};
    wire.clear();
    wire.reserve(256 + response.body().size());

    while (!serializer.is_done()) {
        beast::error_code ec;
        serializer.next(ec, [&](beast::error_code&, const auto& buffers) {
            std::size_t copied = 0;
            for (auto buffer : beast::buffers_range_ref(buffers)) {
                wire.append(static_cast<const char*>(buffer.data()), buffer.size());
                copied += buffer.size();
            }
            if (copied != 0) {
                serializer.consume(copied);
            }
        });
        if (ec) {
            throw boost::system::system_error(ec);
        }
    }
}

template <typename Stream>
boost::asio::awaitable<void> write_error(
    Stream& stream,
    const http::status status,
    const std::string_view message,
    const unsigned version = 11) {
        http::response<http::string_body> res{status, version};
        res.set(http::field::content_type, "text/plain");
        res.body() = std::string(message);
        res.prepare_payload();
        co_await http::async_write(stream, res, net::use_awaitable);
}

template <typename Stream>
boost::asio::awaitable<void> write_response(
    Stream& stream,
    Response& moji_res,
    std::string& response_buffer,
    const bool keep_alive,
    const unsigned version = 11) {
        
    if (moji_res.is_file()) {
        beast::error_code ec;
        http::file_body::value_type body;
        body.open(moji_res.get_file_path().c_str(), beast::file_mode::scan, ec);

        if (ec) {
            co_await write_error(stream, http::status::not_found, "File not found", version);
            co_return;
        }

        const auto size = body.size();
        http::response<http::file_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(static_cast<http::status>(moji_res.get_status()), version)
        };

        for (const auto& [name, value] : moji_res.get_headers()) {
            res.set(name, value);
        }

        res.content_length(size);
        res.keep_alive(keep_alive);
        res.prepare_payload();
        co_await http::async_write(stream, res, net::use_awaitable);
        co_return;
    }

    http::response<http::string_body> res{
        static_cast<http::status>(moji_res.get_status()),
        version};

    for (const auto& [name, value] : moji_res.get_headers()) {
        res.set(name, value);
    }

    res.body() = moji_res.take_body();
    res.keep_alive(keep_alive);
    res.prepare_payload();
    if (res.body().size() <= max_flat_response_body) {
        // The session owns this buffer, and its next request waits for this write.
        flatten_response(res, response_buffer);
        co_await net::async_write(stream, net::buffer(response_buffer), net::use_awaitable);
        // Do not retain an unusually large allocation caused by response headers.
        if (response_buffer.capacity() > max_retained_response_buffer) {
            std::string{}.swap(response_buffer);
        }
        co_return;
    }
    co_await http::async_write(stream, res, net::use_awaitable);
}

} // namespace

template<class Stream>
HttpSession<Stream>::HttpSession(
    Dispatcher& dispatcher,
    const Config& config,
    Stream&& stream)
    : stream_(std::move(stream)),
      dispatcher_(dispatcher),
      config_(config),
      client_ip_(get_client_ip()) {}

template<class Stream>
void HttpSession<Stream>::run() {
    net::co_spawn(
        stream_.get_executor(),
        run_connection(this->shared_from_this()),
        net::detached);
}

template<class Stream>
void HttpSession<Stream>::prepare_read() {
    parser_.emplace();
    parser_->body_limit(config_.max_body_size);
    beast::get_lowest_layer(stream_).expires_after(config_.timeout_seconds);
}

template<class Stream>
net::awaitable<bool> HttpSession<Stream>::read_request() {
    prepare_read();
    beast::error_code ec;
    co_await http::async_read(
        stream_, buffer_, *parser_, net::redirect_error(net::use_awaitable, ec));

    if (!ec) {
        co_return true;
    }

    if (auto error = read_error_response(ec)) {
        try {
            co_await write_error(stream_, error->status, error->message, parser_->get().version());
        } catch (...) {
            // The peer may have disconnected before the error was written.
        }
    }
    co_return false;
}

template<class Stream>
net::awaitable<bool> HttpSession<Stream>::handle_request() {
    auto beast_req = parser_->release();
    bool keep_alive = beast_req.keep_alive();
    const unsigned version = beast_req.version();
    Response moji_res;
    std::optional<ErrorResponse> pending_error;

    try {
        auto req = make_request(std::move(beast_req));
        moji_res = co_await dispatcher_.handle(req, client_ip_, keep_alive);
    } catch (...) {
        pending_error = handler_error_response(std::current_exception());
        if (!pending_error) {
            co_return false;
        }
        keep_alive = false;
    }

    try {
        if (pending_error) {
            co_await write_error(stream_, pending_error->status, pending_error->message, version);
        } else {
            co_await write_response(stream_, moji_res, response_buffer_, keep_alive, version);
        }
    } catch (...) {
        co_return false;
    }
    co_return keep_alive;
}

template<class Stream>
net::awaitable<void> HttpSession<Stream>::run_connection(
    std::shared_ptr<HttpSession<Stream>> self) {
    // This frame owns the session while its read and handler helpers suspend.
    try {
        while (co_await self->read_request()) {
            if (!co_await self->handle_request()) {
                break;
            }
        }
    } catch (const boost::system::system_error& e) {
        if (e.code() != net::error::operation_aborted) {
            std::cerr << "HTTP Session Error: " << e.what() << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "HTTP Session Error: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "HTTP Session Error: unknown exception\n";
    }

    self->shutdown_session();
}

template<class Stream>
void HttpSession<Stream>::shutdown_session() {
    beast::get_lowest_layer(stream_).
        expires_after(std::chrono::seconds(config_.shutdown_timeout));
    StreamTraits<Stream>::shutdown(stream_);
}

template<class Stream>
std::string HttpSession<Stream>::get_client_ip() {
    try {
        return StreamTraits<Stream>::get_socket(stream_).remote_endpoint().address().to_string();
    } catch (...) {
        return "unknown";
    }
}

template class HttpSession<beast::tcp_stream>;

} // namespace moji
