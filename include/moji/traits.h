#pragma once

#include <tuple>

namespace moji {

    template <typename T>
    struct function_traits : public function_traits<decltype(&T::operator())> {
    };

    // function_traits<AsyncLambda> : function_traits<Async<T> (AsyncLambda::*)(Request& const)>
    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const> {
        using args_tuple = std::tuple<Args...>;
        using return_type = ReturnType;
    };

    // function_traits<AsyncLambda> : function_traits<Async<T> (AsyncLambda::*)(Request&)>
    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...)> {
        using args_tuple = std::tuple<Args...>;
        using return_type = ReturnType;
    };

    // function_traits<AsyncLambda> : function_traits<Async<T> (*)(Request&)>
    template <typename ReturnType, typename... Args>
    struct function_traits<ReturnType(*)(Args...)> {
        using args_tuple = std::tuple<Args...>;
        using return_type = ReturnType;
    };

} // namespace moji
