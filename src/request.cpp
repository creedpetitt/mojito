#include <moji/exceptions.h>
#include <moji/request.h>

#include "url.h"

#include <algorithm>
#include <cctype>

namespace moji {

namespace {

bool header_name_equal(const std::string_view lhs, const std::string_view rhs) {
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin(),
               [](const unsigned char a, const unsigned char b) {
               return std::tolower(a) == std::tolower(b);
           });
}

} // namespace

void Request::set_target(std::string_view target) {
    const auto parsed = detail::parse_request_target(target);
    const auto encoded_path = parsed.encoded_path();
    std::string path(encoded_path.data(), encoded_path.size());
    ParamMap query_params;

    for (auto param : parsed.params()) {
        // Preserve the existing policy: ignore keys without '=', last value wins.
        if (param.has_value) {
            query_params.insert_or_assign(std::move(param.key), std::move(param.value));
        }
    }

    path_ = std::move(path);
    query_params_ = std::move(query_params);
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

std::expected<Json, ParseError> Request::try_json() const {
    return Json::parse(body_);
}

Json Request::json() const {
    auto parsed = try_json();
    if (!parsed) {
        throw BadRequest(parsed.error().message);
    }
    return std::move(*parsed);
}

} // namespace moji
