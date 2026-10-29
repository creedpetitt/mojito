#pragma once

#include <moji/async.h>
#include <moji/request.h>
#include <moji/response.h>

#include <variant>
#include <functional>

namespace moji {

using AsyncHandler = std::function<Async<void>(Request&, Response&)>;
using SyncHandler = std::function<void(Request&, Response&)>;

using Handler = std::variant<AsyncHandler, SyncHandler>;

} // namespace moji
