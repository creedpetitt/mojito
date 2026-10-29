#pragma once

#include <moji/parse_error.h>

#include <charconv>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace moji::detail {

// Converts already-decoded parameter text into the requested C++ type.
template<typename T>
std::expected<T, ParseError> parse_parameter(std::string_view text) {
    using Value = std::remove_cvref_t<T>;

    if constexpr (std::is_same_v<Value, std::string>) {
        return std::string(text);
    } else if constexpr (std::is_same_v<Value, bool>) {
        if (text == "true" || text == "1" || text == "yes" || text == "t") return true;
        if (text == "false" || text == "0" || text == "no" || text == "f") return false;
        return std::unexpected(ParseError{
            ParseErrorCode::Invalid,
            "Invalid boolean format: " + std::string(text)
        });
    } else if constexpr (std::is_integral_v<Value> || std::is_floating_point_v<Value>) {
        Value value{};
        const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (ec == std::errc::result_out_of_range) {
            constexpr std::string_view message = std::is_integral_v<Value>
                ? "Integer out of range: " : "Floating point value out of range: ";
            return std::unexpected(ParseError{
                ParseErrorCode::OutOfRange,
                std::string(message) + std::string(text)
            });
        }
        if (ec != std::errc{} || end != text.data() + text.size()) {
            constexpr std::string_view message = std::is_integral_v<Value>
                ? "Invalid integer format: " : "Invalid floating point format: ";
            return std::unexpected(ParseError{
                ParseErrorCode::Invalid,
                std::string(message) + std::string(text)
            });
        }
        return value;
    } else {
        return std::unexpected(ParseError{
            ParseErrorCode::UnsupportedType,
            "Unsupported conversion type"
        });
    }
}

} // namespace moji::detail
