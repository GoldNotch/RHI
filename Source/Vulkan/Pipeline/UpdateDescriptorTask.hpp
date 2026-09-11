#pragma once
#include <functional>
#include <span>

#include <vulkan/vulkan.h>
namespace RHI::vulkan
{
struct Context;
} // namespace RHI::vulkan

namespace RHI::vulkan
{
struct UpdateDescriptorTask final
{
private:
  VkWriteDescriptorSet m_write;
};
} // namespace RHI::vulkan
