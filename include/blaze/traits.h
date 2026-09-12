#pragma once

#include <tuple>
#include <type_traits>

namespace blaze {

    // Function Traits to extract args and return type
    template <typename T>
    struct function_traits : public function_traits<decltype(&T::operator())> {};

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const> {
        using args_tuple = std::tuple<Args...>;
        using return_type = ReturnType;
    };

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...)> {
        using args_tuple = std::tuple<Args...>;
        using return_type = ReturnType;
    };

    template <typename ReturnType, typename... Args>
    struct function_traits<ReturnType(*)(Args...)> {
        using args_tuple = std::tuple<Args...>;
        using return_type = ReturnType;
    };

} // namespace blaze
