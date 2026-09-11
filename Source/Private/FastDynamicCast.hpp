#pragma once
#include <memory>
#include <type_traits>

namespace RHI
{
template<typename Derived, typename Base>
Derived * FastDynamicCast(Base * base) noexcept
{
#ifndef RHI_FAST_DYNAMIC_CAST
  return dynamic_cast<Derived *>(base);
#else
  return static_cast<Derived *>(base);
#endif
}

template<typename Derived, typename Base>
const Derived * FastDynamicCast(const Base * base) noexcept
{
#ifndef RHI_FAST_DYNAMIC_CAST
  return dynamic_cast<const Derived *>(base);
#else
  return static_cast<const Derived *>(base);
#endif
}

template<typename Derived, typename Base>
std::shared_ptr<Derived> FastDynamicCast(std::shared_ptr<Base> base) noexcept
{
#ifndef RHI_FAST_DYNAMIC_CAST
  return std::dynamic_pointer_cast<Derived>(base);
#else
  return std::static_pointer_cast<Derived>(base);
#endif
}
} // namespace RHI
