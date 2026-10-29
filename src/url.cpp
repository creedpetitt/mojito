#include "url.h"

#include <moji/exceptions.h>

#include <boost/url/parse.hpp>
#include <boost/url/pct_string_view.hpp>

namespace moji::detail {

boost::urls::url_view parse_request_target(std::string_view target) {
    const boost::core::string_view text{target.data(), target.size()};
    // Preserve the HTTP asterisk-form target used by OPTIONS requests.
    auto parsed = target == "*"
        ? boost::urls::parse_uri_reference(text)
        : boost::urls::parse_origin_form(text);
    if (!parsed) {
        throw BadRequest("Invalid request target: " + parsed.error().message());
    }
    return *parsed;
}

std::string decode_path_segment(std::string_view segment) {
    auto parsed = boost::urls::make_pct_string_view({segment.data(), segment.size()});
    if (!parsed) {
        throw BadRequest("Invalid path parameter encoding");
    }
    // Default percent-decoding leaves '+' unchanged; query params use their
    // own form-style decoding through url_view::params().
    return parsed->decode();
}

} // namespace moji::detail
