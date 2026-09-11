#pragma once

#include <optional>

#include <RHI.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan::utils
{
struct SemaphoreBuilder final
{
  SemaphoreBuilder & SetTimeline(uint64_t initialValue = 0);
  VkSemaphore Make(const VkDevice & device) const;

private:
  std::optional<uint64_t> m_timelineValue = std::nullopt;
};
} // namespace RHI::vulkan::utils
