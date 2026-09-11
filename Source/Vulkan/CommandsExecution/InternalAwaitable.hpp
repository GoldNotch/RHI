#pragma once
#include <RHI.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct IInternalAwaitable : public IAwaitable
{
  virtual ~IInternalAwaitable() = default;
  virtual VkSemaphore GetSemaphore() const noexcept = 0;
  virtual VkFence GetFence() const noexcept = 0;
};
} // namespace RHI::vulkan
