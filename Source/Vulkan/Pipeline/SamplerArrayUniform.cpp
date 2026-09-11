#include "SamplerArrayUniform.hpp"

#include <Memory/Synchronizer.hpp>
#include <Pipeline/DescriptorBufferLayout.hpp>
#include <Pipeline/Pipeline.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{

SamplerArrayUniform::SamplerArrayUniform(Context & ctx, Pipeline & pipeline, size_t size,
                                         VkDescriptorType type, LayoutIndex index,
                                         uint32_t arrayIndex)
  : BaseDescriptor(ctx, pipeline, type, index, arrayIndex)
{
  m_builder.Reset();
  m_boundTextures.resize(size, dynamic_cast<IInternalTexture *>(ctx.GetNullTexture()));
}

SamplerArrayUniform::~SamplerArrayUniform()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_sampler, nullptr);
}

SamplerArrayUniform::SamplerArrayUniform(SamplerArrayUniform && rhs) noexcept
  : BaseDescriptor(std::move(rhs))
{
  std::swap(rhs.m_boundTextures, m_boundTextures);
  std::swap(rhs.m_sampler, m_sampler);
  std::swap(rhs.m_invalidSampler, m_invalidSampler);
  std::swap(rhs.m_builder, m_builder);
}

SamplerArrayUniform & SamplerArrayUniform::operator=(SamplerArrayUniform && rhs) noexcept
{
  if (this != &rhs)
  {
    BaseDescriptor::operator=(std::move(rhs));
    std::swap(rhs.m_boundTextures, m_boundTextures);
    std::swap(rhs.m_sampler, m_sampler);
    std::swap(rhs.m_invalidSampler, m_invalidSampler);
    std::swap(rhs.m_builder, m_builder);
  }
  return *this;
}

VkSampler SamplerArrayUniform::GetHandle() const noexcept
{
  return m_sampler;
}

void SamplerArrayUniform::UpdateDescriptorSet(std::span<const VkDescriptorSet> sets) const
{
  if (m_shouldUpdate)
  {
    assert(m_sampler);
    std::vector<VkDescriptorImageInfo> infos;
    for (auto * texture : m_boundTextures)
    {
      assert(texture != nullptr);
      auto && imageInfo = infos.emplace_back();
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfo.imageView = texture->GetImageView();
      imageInfo.sampler = m_sampler;
    }
    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.descriptorType = GetDescriptorType();
    writeInfo.dstArrayElement = GetArrayIndex();
    writeInfo.dstBinding = GetBinding();
    writeInfo.descriptorCount = static_cast<uint32_t>(infos.size());
    writeInfo.dstSet = sets[GetSet()];
    writeInfo.pImageInfo = infos.data();
    vkUpdateDescriptorSets(GetContext().GetGpuConnection().GetDevice(), 1, &writeInfo, 0, nullptr);
    m_shouldUpdate = false;
  }
}

void SamplerArrayUniform::CollectResources(std::vector<ResourcePtr> & resources) const
{
  for (auto * texture : m_boundTextures)
  {
    if (texture)
      resources.push_back(texture);
  }
}

void SamplerArrayUniform::SynchroniseResources(SynchronizationFilter filter,
                                               details::CommandBuffer & commands) const
{
  if (FilterSatisfied(filter, SynchronizationFilter::ImageOnly))
  {
    for (auto * texture : m_boundTextures)
    {
      if (texture)
        texture->GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                                      VK_ACCESS_2_SHADER_READ_BIT, commands,
                                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
  }
}

void SamplerArrayUniform::Invalidate()
{
  if (m_invalidSampler || !m_sampler)
  {
    auto new_sampler = m_builder.Make(GetContext().GetGpuConnection().GetDevice());
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "new VkSampler has been created");
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_sampler, nullptr);
    m_sampler = new_sampler;
    m_invalidSampler = false;
  }
}

void SamplerArrayUniform::SetInvalid()
{
  m_invalidSampler = true;
}

void SamplerArrayUniform::AssignImage(uint32_t index, ITexture * image)
{
  m_boundTextures[index] = image ? dynamic_cast<IInternalTexture *>(image)
                                 : dynamic_cast<IInternalTexture *>(GetContext().GetNullTexture());
  m_shouldUpdate = true;
}


void SamplerArrayUniform::SetWrapping(RHI::TextureWrapping uWrap, RHI::TextureWrapping vWrap,
                                      RHI::TextureWrapping wWrap) noexcept
{
  m_builder.SetTextureWrapping(uWrap, vWrap, wWrap);
  m_invalidSampler = true;
}

void SamplerArrayUniform::SetFilter(RHI::TextureFilteration minFilter,
                                    RHI::TextureFilteration magFilter) noexcept
{
  m_builder.SetFilter(minFilter, magFilter);
  m_invalidSampler = true;
}
} // namespace RHI::vulkan
