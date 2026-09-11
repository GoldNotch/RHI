#pragma once
#include <vulkan/vulkan.h>

namespace RHI::vulkan::details
{
struct Synchronizer;
}

namespace RHI::vulkan
{
struct IInternalBuffer
{
  virtual ~IInternalBuffer() = default;
  virtual VkBuffer GetHandle() const noexcept = 0;
  virtual size_t GetSize() const noexcept = 0;
  virtual details::Synchronizer & GetSynchronizer() & noexcept = 0;
};
} // namespace RHI::vulkan
