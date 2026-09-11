#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct EventBuilder final
{
  VkEvent Make(const VkDevice & device, bool signaled) const;
};
} // namespace RHI::vulkan::utils
