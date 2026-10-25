#pragma once

#include <moji/async.h>
#include <moji/request.h>
#include <moji/response.h>

#include <functional>

namespace moji {

using Handler = std::function<Async<void>(Request&, Response&)>;

} // namespace moji
