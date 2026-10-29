#pragma once

#include <boost/url/url_view.hpp>

#include <string>
#include <string_view>

namespace moji::detail {

// Parses an HTTP origin-form target or "*". The returned view borrows target;
// callers must copy any data they retain. Invalid input throws BadRequest.
boost::urls::url_view parse_request_target(std::string_view target);

// Percent-decodes one path segment, preserving literal '+' characters.
std::string decode_path_segment(std::string_view segment);

} // namespace moji::detail
