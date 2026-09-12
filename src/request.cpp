#include <blaze/exceptions.h>
#include <blaze/request.h>
#include <blaze/util/string.h>

#include <boost/json.hpp>

#include <algorithm>
#include <cctype>

namespace blaze {

namespace {

bool header_name_equal(const std::string_view lhs, const std::string_view rhs) {
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin(),
               [](const unsigned char a, const unsigned char b) {
               return std::tolower(a) == std::tolower(b);
           });
}

std::unordered_map<std::string, std::string> parse_query_params(std::string_view query) {
    std::unordered_map<std::string, std::string> params;
    size_t pos = 0;
    while (pos < query.size()) {
        const size_t amp = query.find('&', pos);
        const std::string_view pair = query.substr(pos, amp - pos);
        const size_t eq = pair.find('=');
        if (eq != std::string_view::npos) {
            params[util::url_decode(pair.substr(0, eq))] = util::url_decode(pair.substr(eq + 1));
        }
        if (amp == std::string_view::npos) break;
        pos = amp + 1;
    }
    return params;
}

} // namespace

void Request::set_target(std::string_view target) {
    query_params_.clear();

    const size_t query_pos = target.find('?');
    if (query_pos == std::string_view::npos) {
        path_ = std::string(target);
        return;
    }

    path_ = std::string(target.substr(0, query_pos));
    query_params_ = parse_query_params(target.substr(query_pos + 1));
}

void Request::set_method(const std::string_view method) {
    method_ = std::string(method);
}

void Request::set_body(std::string body) {
    body_ = std::move(body);
}

void Request::set_route_params(ParamMap params) {
    params_ = std::move(params);
}

void Request::set_header(const std::string_view key, const std::string_view value) {
    for (auto& [name, existing_value] : headers_) {
        if (header_name_equal(name, key)) {
            name = std::string(key);
            existing_value = std::string(value);
            return;
        }
    }

    add_header(key, value);
}

void Request::add_header(const std::string_view key, const std::string_view value) {
    headers_.push_back({std::string(key), std::string(value)});
}

std::string_view Request::get_header(const std::string_view key) const {
    for (const auto& [name, value] : headers_) {
        if (header_name_equal(name, key)) {
            return value;
        }
    }
    return "";
}

bool Request::has_header(std::string_view key) const {
    return std::ranges::any_of(headers_, [key](const Header& header) {
        return header_name_equal(header.name, key);
    });
}

std::string Request::url_decode(const std::string_view str) {
    return util::url_decode(str);
}

std::string_view Request::param(std::string_view key) const {
    const auto it = params_.find(std::string(key));
    if (it == params_.end()) {
        throw HttpError(400, "Missing route param: " + std::string(key));
    }
    return it->second;
}

std::optional<std::string_view> Request::query(std::string_view key) const {
    const auto it = query_params_.find(std::string(key));
    if (it == query_params_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::expected<Json, util::ParseError> Request::try_json() const {
    boost::system::error_code ec;
    auto value = boost::json::parse(body_, ec);
    if (ec) {
        return std::unexpected(util::ParseError{
            util::ParseErrorCode::Invalid,
            "Invalid JSON in request body: " + ec.message()
        });
    }

    return Json(std::move(value));
}

Json Request::json() const {
    auto parsed = try_json();
    if (!parsed) {
        throw BadRequest(parsed.error().message);
    }
    return std::move(*parsed);
}

} // namespace blaze
