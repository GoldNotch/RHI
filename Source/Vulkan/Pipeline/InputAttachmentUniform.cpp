#include "InputAttachmentUniform.hpp"

#include <Memory/Synchronizer.hpp>
#include <Pipeline/DescriptorBufferLayout.hpp>
#include <Pipeline/Pipeline.hpp>
#include <Private/FastDynamicCast.hpp>
#include <RenderPass/RenderPass.hpp>
#include <RenderPass/RenderTarget.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace RHI::vulkan
{
InputAttachmentUniform::InputAttachmentUniform(Context & ctx, Pipeline & pipeline,
                                               LayoutIndex index, uint32_t attachmentIdx)
  : BaseDescriptor(ctx, pipeline, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, index, 0)
  , m_attachmentIndex(attachmentIdx)
{
}

void InputAttachmentUniform::UpdateDescriptorSet(std::span<const VkDescriptorSet> sets) const
{
  VkDescriptorImageInfo imageInfo{};
  if (RenderPass * renderPass = GetPipeline().GetBindPoint())
  {
    auto * target = renderPass->GetActiveRenderTarget();
    assert(target);
    imageInfo.imageView = target->GetImageViews()[m_attachmentIndex];
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
  }

  VkWriteDescriptorSet writeInfo{};
  writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeInfo.descriptorType = GetDescriptorType();
  writeInfo.dstArrayElement = GetArrayIndex();
  writeInfo.dstBinding = GetBinding();
  writeInfo.dstSet = sets[GetSet()];
  writeInfo.descriptorCount = 1;
  writeInfo.pImageInfo = &imageInfo;
  vkUpdateDescriptorSets(GetContext().GetGpuConnection().GetDevice(), 1, &writeInfo, 0, nullptr);
}

void InputAttachmentUniform::CollectResources(std::vector<ResourcePtr> & resources) const
{
}

void InputAttachmentUniform::SynchroniseResources(details::CommandBuffer & commands) const
{
}

void InputAttachmentUniform::Invalidate()
{
}

void InputAttachmentUniform::SetInvalid()
{
}

} // namespace RHI::vulkan
