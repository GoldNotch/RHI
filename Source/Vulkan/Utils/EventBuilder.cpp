#include "EventBuilder.hpp"


namespace RHI::vulkan::utils
{

VkEvent EventBuilder::Make(const VkDevice & device, bool signaled) const
{
  VkEvent result = VK_NULL_HANDLE;
  VkEventCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

  // Don't use createSemaphore in dispatchTable because it's broken
  if (vkCreateEvent(device, &info, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("failed to create vkEvent");

  if (signaled)
    vkSetEvent(device, result);
  return result;
}

} // namespace RHI::vulkan::utils
