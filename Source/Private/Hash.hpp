#pragma once
#include <cstdint>

namespace RHI::utils
{
template<typename T>
inline void HashCombine(std::size_t & seed, const T & value)
{
  std::hash<T> hasher;
  // Boost formula
  seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
} // namespace RHI::utils
