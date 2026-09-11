#include "DescriptorsBuffer.hpp"

#include <algorithm>
#include <array>
#include <numeric>

#include <Memory/BufferGPU.hpp>
#include <Pipeline/DescriptorBufferLayout.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>


namespace RHI::vulkan::details
{

constexpr RHI::BufferGPUUsage DescriptorType2BufferUsage(VkDescriptorType type)
{
  switch (type)
  {
    /*case VK_DESCRIPTOR_TYPE_SAMPLER:
      return VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;*/
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      return BufferGPUUsage::UniformBuffer;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      return BufferGPUUsage::StorageBuffer;
    default:
      throw std::runtime_error("Failed to cast DescriptorType to BufferUsage");
  }
}
} // namespace RHI::vulkan::details


namespace RHI::vulkan
{

DescriptorBuffer::DescriptorBuffer(Context & ctx)
  : OwnedBy<Context>(ctx)
{
}

DescriptorBuffer::~DescriptorBuffer()
{
  vkDestroyDescriptorPool(GetContext().GetGpuConnection().GetDevice(), m_pool, nullptr);
}

DescriptorBuffer::DescriptorBuffer(DescriptorBuffer && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
{
  std::swap(m_pool, rhs.m_pool);
  std::swap(m_sets, rhs.m_sets);
}

DescriptorBuffer & DescriptorBuffer::operator=(DescriptorBuffer && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    std::swap(m_pool, rhs.m_pool);
    std::swap(m_sets, rhs.m_sets);
  }
  return *this;
}

void DescriptorBuffer::Invalidate(const DescriptorBufferLayout & layout)
{
  if (m_cachedLayoutsHash != layout.GetLayoutsHash())
  {
    auto [newPool, newSets] = layout.AllocDescriptorSets();
    vkDestroyDescriptorPool(GetContext().GetGpuConnection().GetDevice(), m_pool, nullptr);
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG,
                     "VkDescriptorPool({}) & Sets have been rebuilt - {}",
                     static_cast<void *>(m_pool), static_cast<void *>(newPool));
    m_pool = newPool;
    m_sets = std::move(newSets);
    m_cachedLayoutsHash = layout.GetLayoutsHash();
  }
}

std::span<const VkDescriptorSet> DescriptorBuffer::GetSets() const noexcept
{
  return m_sets;
}

} // namespace RHI::vulkan
