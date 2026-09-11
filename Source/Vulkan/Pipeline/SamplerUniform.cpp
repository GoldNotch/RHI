#include "SamplerUniform.hpp"

#include <Memory/Synchronizer.hpp>
#include <Pipeline/DescriptorBufferLayout.hpp>
#include <Pipeline/Pipeline.hpp>
#include <Private/FastDynamicCast.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

SamplerUniform::SamplerUniform(Context & ctx, Pipeline & pipeline, VkDescriptorType type,
                               LayoutIndex index, uint32_t arrayIndex)
  : BaseDescriptor(ctx, pipeline, type, index, arrayIndex)
  , ISamplerUniformDescriptor()
{
  m_builder.Reset();
  AssignImage(nullptr);
}

SamplerUniform::~SamplerUniform()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_sampler, nullptr);
}

SamplerUniform::SamplerUniform(SamplerUniform && rhs) noexcept
  : BaseDescriptor(std::move(rhs))
  , ISamplerUniformDescriptor()
{
  std::swap(rhs.m_boundTexture, m_boundTexture);
  std::swap(rhs.m_sampler, m_sampler);
  std::swap(rhs.m_invalidSampler, m_invalidSampler);
  std::swap(rhs.m_builder, m_builder);
}

SamplerUniform & SamplerUniform::operator=(SamplerUniform && rhs) noexcept
{
  if (this != &rhs)
  {
    BaseDescriptor::operator=(std::move(rhs));
    std::swap(rhs.m_boundTexture, m_boundTexture);
    std::swap(rhs.m_sampler, m_sampler);
    std::swap(rhs.m_invalidSampler, m_invalidSampler);
    std::swap(rhs.m_builder, m_builder);
  }
  return *this;
}

VkSampler SamplerUniform::GetHandle() const noexcept
{
  return m_sampler;
}

void SamplerUniform::CollectResources(std::vector<ResourcePtr> & resources) const
{
  if (m_boundTexture)
    resources.push_back(m_boundTexture);
}

void SamplerUniform::SynchroniseResources(SynchronizationFilter filter, details::CommandBuffer & commands) const
{
  if (m_boundTexture && FilterSatisfied(filter, SynchronizationFilter::ImageOnly))
  {
    m_boundTexture->GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                                         VK_ACCESS_2_SHADER_READ_BIT, commands,
                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}


void SamplerUniform::Invalidate()
{
  if (m_invalidSampler || !m_sampler)
  {
    auto new_sampler = m_builder.Make(GetContext().GetGpuConnection().GetDevice());
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_sampler, nullptr);
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkSampler({}) has been rebuilt - {}",
                     static_cast<void *>(m_sampler), static_cast<void *>(new_sampler));
    m_sampler = new_sampler;
    m_invalidSampler = false;
  }
}

void SamplerUniform::SetInvalid()
{
  m_invalidSampler = true;
}

void SamplerUniform::AssignImage(ITexture * image)
{
  m_boundTexture = image ? FastDynamicCast<IInternalTexture>(image)
                         : FastDynamicCast<IInternalTexture>(GetContext().GetNullTexture());
  m_shouldUpdate = true;
}

void SamplerUniform::UpdateDescriptorSet(std::span<const VkDescriptorSet> sets) const
{
  if (m_shouldUpdate)
  {
    assert(m_sampler);
    assert(m_boundTexture);
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_boundTexture->GetImageView();
    imageInfo.sampler = m_sampler;
    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.descriptorType = GetDescriptorType();
    writeInfo.dstArrayElement = GetArrayIndex();
    writeInfo.dstBinding = GetBinding();
    writeInfo.descriptorCount = 1;
    writeInfo.dstSet = sets[GetSet()];
    writeInfo.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(GetContext().GetGpuConnection().GetDevice(), 1, &writeInfo, 0, nullptr);
    m_shouldUpdate = false;
  }
}

void SamplerUniform::SetWrapping(RHI::TextureWrapping uWrap, RHI::TextureWrapping vWrap,
                                 RHI::TextureWrapping wWrap) noexcept
{
  m_builder.SetTextureWrapping(uWrap, vWrap, wWrap);
  m_invalidSampler = true;
}

void SamplerUniform::SetFilter(RHI::TextureFilteration minFilter,
                               RHI::TextureFilteration magFilter) noexcept
{
  m_builder.SetFilter(minFilter, magFilter);
  m_invalidSampler = true;
}
} // namespace RHI::vulkan
