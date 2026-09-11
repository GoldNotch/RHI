#pragma once

namespace std
{

// 1. Define the overload pattern helper
template<class... Ts>
struct overload : Ts...
{
  using Ts::operator()...;
};
// Note: If you are using C++20 or later, the explicit deduction guide below is optional
template<class... Ts>
overload(Ts...) -> overload<Ts...>;

} // namespace std
