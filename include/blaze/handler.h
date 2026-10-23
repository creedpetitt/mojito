#pragma once

#include <blaze/async.h>
#include <blaze/request.h>
#include <blaze/response.h>

#include <functional>

namespace blaze {

using Handler = std::function<Async<void>(Request&, Response&)>;

} // namespace blaze
