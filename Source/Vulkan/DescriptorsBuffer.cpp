#include "DescriptorsBuffer.hpp"

#include <algorithm>
#include <array>
#include <numeric>

#include "Utils/Builders.hpp"


namespace RHI::vulkan
{
namespace details
{

vk::DescriptorPool CreateDescriptorPool(const Context & ctx,
                                        const DescriptorBuffer::CapacityDistribution & distribution)
{
  std::vector<VkDescriptorPoolSize> poolSizes;
  for (auto [type, count] : distribution)
  {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = type;
    poolSize.descriptorCount = count;
    poolSizes.push_back(poolSize);
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = std::accumulate(poolSizes.begin(), poolSizes.end(), 0u,
                                     [](uint32_t acc, const VkDescriptorPoolSize & pool_size)
                                     { return acc + pool_size.descriptorCount; });

  VkDescriptorPool c_pool;
  if (vkCreateDescriptorPool(ctx.GetDevice(), &poolInfo, nullptr, &c_pool) != VK_SUCCESS)
    throw std::runtime_error("failed to create descriptor pool!");
  return vk::DescriptorPool(c_pool);
}

vk::DescriptorSet CreateDescriptorSet(const Context & ctx, VkDescriptorPool pool,
                                      VkDescriptorSetLayout layout)
{
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;

  VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
  if (vkAllocateDescriptorSets(ctx.GetDevice(), &allocInfo, &descriptor_set) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate descriptor sets!");
  return vk::DescriptorSet(descriptor_set);
}


void LinkBufferToDescriptor(const Context & ctx, VkDescriptorSet set, VkDescriptorType type,
                            uint32_t binding, const IBufferGPU & buffer)
{
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = reinterpret_cast<VkBuffer>(buffer.GetHandle());
  bufferInfo.range = buffer.Size();
  bufferInfo.offset = 0;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = set;
  descriptorWrite.dstBinding = binding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = type;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;
  descriptorWrite.pImageInfo = nullptr;       // Optional
  descriptorWrite.pTexelBufferView = nullptr; // Optional

  vkUpdateDescriptorSets(ctx.GetDevice(), 1, &descriptorWrite, 0, nullptr);
}

RHI::BufferGPUUsage DescriptorType2BufferUsage(VkDescriptorType type)
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
} // namespace details


DescriptorBuffer::DescriptorBuffer(const Context & ctx)
  : m_owner(ctx)
{
  m_layoutBuilder = std::make_unique<details::DescriptorSetLayoutBuilder>();
}

DescriptorBuffer::~DescriptorBuffer()
{
  if (m_layout)
    vkDestroyDescriptorSetLayout(m_owner.GetDevice(), m_layout, nullptr);
  if (m_pool)
    vkDestroyDescriptorPool(m_owner.GetDevice(), m_pool, nullptr);
}

void DescriptorBuffer::Invalidate()
{
  if (!m_buffers.empty())
  {
    if (m_invalidLayout || !m_layout)
    {
      auto new_layout = m_layoutBuilder->Make(m_owner.GetDevice());
      if (!!m_layout)
        vkDestroyDescriptorSetLayout(m_owner.GetDevice(), m_layout, nullptr);
      m_layout = new_layout;
      m_invalidLayout = false;
      m_invalidSet = true;
    }

    if (m_invalidPool || !m_pool)
    {
      auto new_pool = details::CreateDescriptorPool(m_owner, m_capacity);
      if (m_pool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(m_owner.GetDevice(), m_pool, nullptr);
      m_pool = new_pool;
      m_invalidPool = false;
      m_invalidSet = true;
    }

    if (m_invalidSet || !m_set)
    {
      auto new_set = details::CreateDescriptorSet(m_owner, m_pool, m_layout);
      m_set = new_set;
      for (uint32_t slot = 0; slot < m_buffers.size(); ++slot)
      {
        auto && [type, bufPtr] = m_buffers[slot];
        details::LinkBufferToDescriptor(m_owner, m_set, type, slot, *bufPtr);
      }
      m_invalidSet = false;
    }
  }
}

IBufferGPU * DescriptorBuffer::DeclareUniform(uint32_t binding, ShaderType shaderStage,
                                              uint32_t size)
{
  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  m_layoutBuilder->DeclareUniform(binding, shaderStage);
  m_invalidLayout = true;

  m_capacity[type]++;
  auto && new_buffer = m_owner.AllocBuffer(size, details::DescriptorType2BufferUsage(type), true);
  auto && descriptedBuffer = m_buffers.emplace_back(type, std::move(new_buffer));
  return descriptedBuffer.second.get();
}

void DescriptorBuffer::Bind(const vk::CommandBuffer & buffer, vk::PipelineLayout pipelineLayout,
                            VkPipelineBindPoint bindPoint)
{
  if (!m_buffers.empty())
  {
    const VkDescriptorSet set = m_set;
    vkCmdBindDescriptorSets(buffer, bindPoint, pipelineLayout, 0, 1, &set, 1, nullptr);
  }
}

vk::DescriptorSetLayout DescriptorBuffer::GetLayoutHandle() const noexcept
{
  return m_layout;
}

vk::DescriptorSet DescriptorBuffer::GetHandle() const noexcept
{
  return m_set;
}

} // namespace RHI::vulkan
