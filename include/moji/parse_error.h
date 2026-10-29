#pragma once

#include <string>

namespace moji {

enum class ParseErrorCode {
    Invalid,
    OutOfRange,
    UnsupportedType
};

struct ParseError {
    ParseErrorCode code;
    std::string message;
};

} // namespace moji
