#pragma once

#include <moji/json.h>
#include <moji/header.h>
#include <moji/util/string.h>

#include <any>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace moji {

class Request {
public:
    using ParamMap = std::unordered_map<std::string, std::string>;

    std::string_view method() const noexcept { return method_; }
    std::string_view path() const noexcept { return path_; }
    std::string_view body() const noexcept { return body_; }

    void set_method(std::string_view method);
    void set_target(std::string_view target);
    void set_body(std::string body);
    void set_route_params(ParamMap params);

    void set_header(std::string_view key, std::string_view value);
    void add_header(std::string_view key, std::string_view value);
    std::string_view get_header(std::string_view key) const;
    bool has_header(std::string_view key) const;
    const std::vector<Header>& headers() const { return headers_; }

    static std::string url_decode(std::string_view str);

    const ParamMap& params() const noexcept { return params_; }
    const ParamMap& query_params() const noexcept { return query_params_; }

    std::string_view param(std::string_view key) const;

    template<typename T>
    T param(std::string_view key) const {
        const auto value = param(key);
        auto parsed = util::parse_value<T>(value);
        if (!parsed) {
            throw HttpError(400, parsed.error().message);
        }
        return std::move(*parsed);
    }

    std::optional<std::string_view> query(std::string_view key) const;

    template<typename T>
    std::optional<T> query(std::string_view key) const {
        const auto value = query(key);
        if (!value) {
            return std::nullopt;
        }

        auto parsed = util::parse_value<T>(*value);
        if (!parsed) {
            throw HttpError(400, parsed.error().message);
        }
        return std::move(*parsed);
    }

    template<typename T>
    T query_or(std::string_view key, T default_value) const {
        auto value = query<T>(key);
        if (!value) {
            return default_value;
        }
        return std::move(*value);
    }

    std::expected<Json, util::ParseError> try_json() const;
    Json json() const;

    template<typename T>
    void set_state(std::string_view key, T&& value) {
        state_[std::string(key)] = std::make_any<std::remove_cvref_t<T>>(std::forward<T>(value));
    }

    template<typename T>
    std::optional<T> state(std::string_view key) const {
        const auto it = state_.find(std::string(key));
        if (it == state_.end()) return std::nullopt;
        try {
            return std::any_cast<T>(it->second);
        } catch (...) {
            return std::nullopt;
        }
    }

private:
    std::string method_;
    std::string path_;
    std::string body_;
    ParamMap params_;
    ParamMap query_params_;
    std::vector<Header> headers_;
    std::unordered_map<std::string, std::any> state_;
};

} // namespace moji
