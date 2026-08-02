#pragma once
#include <optional>
#include <variant>
#include <functional>
#include <stack>
#include <type_traits>

using std::optional;
using std::stack;

template <typename... Ts>
using sumtype = std::variant<Ts...>;

template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

template<class... Functors, class... Ts>
inline constexpr decltype(auto) match(const sumtype<Ts...>& target, Functors... functor) {
    return std::visit(overload{functor...}, target);
}

template<class... Functors, class... Ts>
inline constexpr decltype(auto) match(sumtype<Ts...>& target, Functors... functor) {
    return std::visit(overload{functor...}, target);
}

using std::get_if;
using std::get;