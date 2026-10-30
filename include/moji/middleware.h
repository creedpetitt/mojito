#pragma once

#include <moji/async.h>
#include <moji/request.h>
#include <moji/response.h>

#include <functional>

namespace moji {
    
using Next = std::function<Async<void>()>;
using Middleware = std::function<Async<void>(Request&, Response&, Next)>;

} // namespace moji
