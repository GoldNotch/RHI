#pragma once

#define RESTRICTED_COPY(ClassName)                                                                 \
private:                                                                                           \
  ClassName(const ClassName &) = delete;                                                           \
  ClassName & operator=(const ClassName &) = delete;

#define DECLARE_BIT_OPERATIONS_FOR_ENUM(EnumName)                                                  \
  constexpr inline EnumName operator|(EnumName t1, EnumName t2) noexcept                           \
  {                                                                                                \
    return static_cast<EnumName>(static_cast<uint32_t>(t1) | static_cast<uint32_t>(t2));           \
  }                                                                                                \
  constexpr inline EnumName operator&(EnumName t1, EnumName t2) noexcept                           \
  {                                                                                                \
    return static_cast<EnumName>(static_cast<uint32_t>(t1) & static_cast<uint32_t>(t2));           \
  }                                                                                                \
  constexpr inline EnumName operator&=(EnumName t1, EnumName t2) noexcept                          \
  {                                                                                                \
    return static_cast<EnumName>(static_cast<uint32_t>(t1) & static_cast<uint32_t>(t2));           \
  }                                                                                                \
  constexpr inline EnumName operator|=(EnumName t1, EnumName t2) noexcept                          \
  {                                                                                                \
    return static_cast<EnumName>(static_cast<uint32_t>(t1) | static_cast<uint32_t>(t2));           \
  }

namespace RHI::utils
{
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
inline constexpr T bit(int i)
{
  return static_cast<T>(1) << i;
}
} // namespace RHI::utils
