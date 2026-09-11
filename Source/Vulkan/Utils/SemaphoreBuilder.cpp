#include "SemaphoreBuilder.hpp"

#include <stdexcept>

namespace RHI::vulkan::utils
{
SemaphoreBuilder & SemaphoreBuilder::SetTimeline(uint64_t initialValue)
{
  m_timelineValue = initialValue;
  return *this;
}

VkSemaphore SemaphoreBuilder::Make(const VkDevice & device) const
{
  VkSemaphore result = VK_NULL_HANDLE;
  VkSemaphoreCreateInfo info{};
  VkSemaphoreTypeCreateInfo typeInfo{};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
  if (m_timelineValue.has_value())
  {
    typeInfo.initialValue = *m_timelineValue;
    typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    info.pNext = &typeInfo;
  }
  // Don't use createSemaphore in dispatchTable because it's broken
  if (vkCreateSemaphore(device, &info, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("failed to create semaphore");
  return VkSemaphore(result);
}

} // namespace RHI::vulkan::utils
