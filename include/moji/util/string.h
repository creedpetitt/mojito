#pragma once

#include <moji/exceptions.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <charconv>
#include <expected>
#include <system_error>
#include <utility>

namespace moji::util {

enum class ParseErrorCode {
    Invalid,
    OutOfRange,
    UnsupportedType
};

struct ParseError {
    ParseErrorCode code;
    std::string message;
};

/**
 * @brief Decodes a URL-encoded string (e.g., %20 to space).
 * Handles both '+' and '%xx' encodings.
 */
std::string url_decode(std::string_view str);

template<typename T>
std::expected<T, ParseError> parse_value(std::string_view s) {
    using PureT = std::remove_cvref_t<T>;

    if constexpr (std::is_same_v<PureT, std::string>) {
        return std::string(s);
    } else if constexpr (std::is_same_v<PureT, bool>) {
        if (s == "true" || s == "1" || s == "yes" || s == "t") return true;
        if (s == "false" || s == "0" || s == "no" || s == "f") return false;
        return std::unexpected(ParseError{
            ParseErrorCode::Invalid,
            "Invalid boolean format: " + std::string(s)
        });
    } else if constexpr (std::is_integral_v<PureT>) {
        PureT val{};
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
        if (ec == std::errc::result_out_of_range) {
            return std::unexpected(ParseError{
                ParseErrorCode::OutOfRange,
                "Integer out of range: " + std::string(s)
            });
        }
        if (ec != std::errc() || ptr != s.data() + s.size()) {
            return std::unexpected(ParseError{
                ParseErrorCode::Invalid,
                "Invalid integer format: " + std::string(s)
            });
        }
        return val;
    } else if constexpr (std::is_floating_point_v<PureT>) {
        PureT val{};
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
        if (ec == std::errc::result_out_of_range) {
            return std::unexpected(ParseError{
                ParseErrorCode::OutOfRange,
                "Floating point value out of range: " + std::string(s)
            });
        }
        if (ec != std::errc() || ptr != s.data() + s.size()) {
            return std::unexpected(ParseError{
                ParseErrorCode::Invalid,
                "Invalid floating point format: " + std::string(s)
            });
        }
        return val;
    } else {
        return std::unexpected(ParseError{
            ParseErrorCode::UnsupportedType,
            "Unsupported conversion type"
        });
    }
}

} // namespace moji::util

namespace moji {

    using util::parse_value;
}
