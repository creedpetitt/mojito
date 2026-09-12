#pragma once

#include <blaze/request.h>
#include <blaze/response.h>
#include <blaze/traits.h>

#include <functional>
#include <tuple>
#include <type_traits>

namespace blaze {

namespace detail {

    template<typename>
    inline constexpr bool always_false_v = false;

    template<typename ArgType>
    decltype(auto) resolve_handler_arg(Request& req, Response& res) {
        using PureType = std::remove_cvref_t<ArgType>;

        if constexpr (std::is_lvalue_reference_v<ArgType> && std::is_same_v<PureType, Request>) {
            if constexpr (std::is_const_v<std::remove_reference_t<ArgType>>) {
                return static_cast<const Request&>(req);
            } else {
                return static_cast<Request&>(req);
            }
        } else if constexpr (
            std::is_lvalue_reference_v<ArgType> &&
            std::is_same_v<PureType, Response> &&
            !std::is_const_v<std::remove_reference_t<ArgType>>
        ) {
            return res;
        } else {
            static_assert(always_false_v<ArgType>,
                "Unsupported handler argument. Use Request&, const Request&, or Response&.");
        }
    }
}

template<typename Func, typename Tuple, size_t... Is>
auto call_with_deps_impl(Func& func, Request& req, Response& res, std::index_sequence<Is...>) {
    return std::invoke(
        func,
        detail::resolve_handler_arg<std::tuple_element_t<Is, Tuple>>(req, res)...
    );
}

template<typename Func>
auto inject_and_call(Func& func, Request& req, Response& res) {
    using Traits = function_traits<Func>;
    using ArgsTuple = Traits::args_tuple;
    
    return call_with_deps_impl<Func, ArgsTuple>(
        func, req, res,
        std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{}
    );
}

} // namespace blaze
