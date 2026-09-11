#include "BufferUniform.hpp"

#include <Memory/Synchronizer.hpp>
#include <Pipeline/DescriptorBufferLayout.hpp>
#include <Pipeline/Pipeline.hpp>
#include <Private/FastDynamicCast.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
BufferUniform::BufferUniform(Context & ctx, Pipeline & pipeline, VkDescriptorType type,
                             LayoutIndex index, uint32_t arrayIndex)
  : BaseDescriptor(ctx, pipeline, type, index, arrayIndex)
  , IBufferUniformDescriptor()
{
}

BufferUniform::BufferUniform(BufferUniform && rhs) noexcept
  : BaseDescriptor(std::move(rhs))
{
  std::swap(m_buffer, rhs.m_buffer);
  std::swap(m_offset, rhs.m_offset);
}

BufferUniform & BufferUniform::operator=(BufferUniform && rhs) noexcept
{
  if (this != &rhs)
  {
    BaseDescriptor::operator=(std::move(rhs));
    std::swap(m_buffer, rhs.m_buffer);
    std::swap(m_offset, rhs.m_offset);
  }
  return *this;
}

void BufferUniform::UpdateDescriptorSet(std::span<const VkDescriptorSet> sets) const
{
  if (m_shouldUpdate)
  {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_buffer->GetHandle();
    bufferInfo.range = m_buffer->GetSize();
    bufferInfo.offset = m_offset;
    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.descriptorType = GetDescriptorType();
    writeInfo.dstArrayElement = GetArrayIndex();
    writeInfo.dstBinding = GetBinding();
    writeInfo.descriptorCount = 1;
    writeInfo.dstSet = sets[GetSet()];
    writeInfo.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(GetContext().GetGpuConnection().GetDevice(), 1, &writeInfo, 0, nullptr);
    m_shouldUpdate = false;
  }
}

void BufferUniform::AssignBuffer(IBufferGPU * buffer, size_t offset)
{
  Invalidate();
  m_buffer = FastDynamicCast<IInternalBuffer>(buffer);
  m_offset = offset;
  m_shouldUpdate = true;
}

bool BufferUniform::IsBufferAssigned() const noexcept
{
  return m_buffer;
}

void BufferUniform::CollectResources(std::vector<ResourcePtr> & resources) const
{
  if (m_buffer)
    resources.push_back(m_buffer);
}

void BufferUniform::SynchroniseResources(SynchronizationFilter filter, details::CommandBuffer & commands) const
{
  if (m_buffer && FilterSatisfied(filter, SynchronizationFilter::BufferOnly))
  {
    m_buffer->GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                                   VK_ACCESS_2_SHADER_READ_BIT, commands,
                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}

void BufferUniform::Invalidate()
{
}

void BufferUniform::SetInvalid()
{
}

} // namespace RHI::vulkan
