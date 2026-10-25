#pragma once

#include <moji/exceptions.h>
#include <moji/util/string.h>

#include <concepts>
#include <cstdint>
#include <expected>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace moji {

namespace detail {
template<typename>
inline constexpr bool unsupported_json_as_v = false;
}

class Json {
public:
    enum class Type {
        Null,
        Bool,
        Int,
        UInt,
        Double,
        String,
        Array,
        Object
    };

    using Member = std::pair<std::string_view, Json>;

    Json();
    Json(std::nullptr_t);
    Json(bool value);
    Json(int value);
    Json(unsigned int value);
    Json(std::int64_t value);
    Json(std::uint64_t value);
    Json(double value);
    Json(const char* value);
    Json(std::string value);
    Json(std::string_view value);
    Json(std::initializer_list<Member> members);

    template<std::signed_integral T>
        requires (!std::same_as<std::remove_cvref_t<T>, bool>)
    Json(T value) : Json(static_cast<std::int64_t>(value)) {}

    template<std::unsigned_integral T>
        requires (!std::same_as<std::remove_cvref_t<T>, bool>)
    Json(T value) : Json(static_cast<std::uint64_t>(value)) {}

    Json(const Json& other);
    Json(Json&& other);
    Json& operator=(const Json& other);
    Json& operator=(Json&& other);
    ~Json();

    static Json object(std::initializer_list<Member> members);
    static Json array(std::initializer_list<Json> items);

    template<typename... Args>
    static Json array(Args&&... args) {
        return array({Json(std::forward<Args>(args))...});
    }

    static std::expected<Json, util::ParseError> parse(std::string_view text);

    Type type() const noexcept;
    bool is_null() const noexcept;
    bool is_bool() const noexcept;
    bool is_number() const noexcept;
    bool is_string() const noexcept;
    bool is_array() const noexcept;
    bool is_object() const noexcept;

    Json operator[](size_t idx) const;
    Json operator[](std::string_view key) const;
    Json operator[](const char* key) const { return (*this)[std::string_view(key)]; }

    bool has(std::string_view key) const;
    size_t size() const noexcept;
    bool empty() const noexcept { return size() == 0; }

    void set(std::string_view key, Json value);

    template<typename T>
    T as() const {
        static_assert(detail::unsupported_json_as_v<T>, "Unsupported Json::as<T>() type.");
    }

    template<typename T>
    std::optional<T> try_as() const {
        try {
            return as<T>();
        } catch (const BadRequest&) {
            return std::nullopt;
        }
    }

    template<typename T>
    std::optional<T> try_get(std::string_view key) const {
        return (*this)[key].template try_as<T>();
    }

    explicit operator std::string() const;
    explicit operator int() const;

    std::string dump() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

template<> bool Json::as<bool>() const;
template<> std::string Json::as<std::string>() const;
template<> int Json::as<int>() const;
template<> unsigned int Json::as<unsigned int>() const;
template<> std::int64_t Json::as<std::int64_t>() const;
template<> std::uint64_t Json::as<std::uint64_t>() const;
template<> double Json::as<double>() const;
template<> Json Json::as<Json>() const;

} // namespace moji
