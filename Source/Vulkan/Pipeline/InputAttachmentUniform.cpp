#include "InputAttachmentUniform.hpp"

#include <Memory/Synchronizer.hpp>
#include <Pipeline/DescriptorBufferLayout.hpp>
#include <Pipeline/Pipeline.hpp>
#include <Private/FastDynamicCast.hpp>
#include <RenderPass/Framebuffer.hpp>
#include <RenderPass/RenderPass.hpp>
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
    auto attachment = renderPass->GetFramebuffer().GetAttachment(m_attachmentIndex);
    if (!attachment)
      return;
    imageInfo.imageView = attachment->GetImageView();
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

void InputAttachmentUniform::SynchroniseResources(SynchronizationFilter filter,
                                                  details::CommandBuffer & commands) const
{
  if (FilterSatisfied(filter, SynchronizationFilter::ImageOnly))
  {
    if (RenderPass * renderPass = GetPipeline().GetBindPoint())
    {
      auto attachment = renderPass->GetFramebuffer().GetAttachment(m_attachmentIndex);
      attachment->GetSynchronizer().RequireSynchronize(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                                       VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT,
                                                       commands,
                                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
  }
}

void InputAttachmentUniform::Invalidate()
{
}

void InputAttachmentUniform::SetInvalid()
{
}

} // namespace RHI::vulkan
